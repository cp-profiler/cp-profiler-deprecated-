
namespace cpprofiler {
namespace analysis {

struct ChildInfo {
  int alt;
  VisualNode* node;
};

struct PosInGroups {
  int group_idx;
  int inner_idx;
};

class Group {
  static int counter;
public:
  int splitter = 0;
  std::vector<ChildInfo> items;
  Group() = default;
  Group(const Group& other) {
    counter++;
    // qDebug() << "Group copied times: " << counter;
    splitter = other.splitter;
    items = other.items;
  }
  Group(int s, const std::vector<ChildInfo>& i) : splitter(s), items(i) {}
};

namespace detail {

int getSubtreeHeight(const VisualNode* n, const NodeAllocator& na,
                     std::vector<Group>& groups) {
  int max = 0;

  int kids = n->getNumberOfChildren();

  if (kids == 0) {
    return 1;
  }

  for (int i = 0; i < kids; ++i) {
    auto kid = n->getChild(na, i);
    int h = getSubtreeHeight(kid, na, groups);
    auto& group_items = groups[h].items;
    group_items.push_back({i, kid});
    if (h > max) {
      max = h;
    }
  }

  return max + 1;
}

/// Groups nodes by height of their underlying subtree
/// TODO(maxim): NodeTree should be able to tell its depth
static std::vector<Group> groupByHeight(const TreeCanvas& tc, const NodeTree& nt) {
  int max_depth = tc.get_stats().maxDepth;

  /// start from 1 for convenience
  std::vector<Group> groups(max_depth + 1);

  auto& na = nt.getNA();
  auto* root = nt.getRootNode();

  getSubtreeHeight(root, na, groups);

  /// edge case of a root node
  groups[groups.size() - 1].items.push_back({-1, root});

  return groups;
}

void splitGroups(
    std::vector<Group>& groups, const std::vector<int>& g_to_split,
    std::unordered_map<const VisualNode*, PosInGroups>& node2groupID) {
  for (auto i = 0u; i < g_to_split.size(); ++i) {
    auto g_idx = g_to_split[i];
    auto& g = groups[g_idx];
    if (g.splitter == 0 || g.splitter == (int)g.items.size()) {
      g.splitter = 0;
      /// ----- don't need to split the group -----
      continue;
    }
    /// ----- need to split the group -----
    perfHelper.accumulate("actual splitting");
    auto it_splitter = begin(g.items) + g.splitter;

    Group new_group2{0, {it_splitter, end(g.items)}};

    g.items.erase(it_splitter, end(g.items));
    g.splitter = 0;

    /// ----- change group_id for nodes in the second group -----
    for (auto j = 0u; j < new_group2.items.size(); ++j) {
      auto* node = new_group2.items[j].node;
      node2groupID[node].group_idx = groups.size();
      node2groupID[node].inner_idx = j;
    }
    groups.push_back(std::move(new_group2));
    perfHelper.end("actual splitting");

    // ++i;
    // qDebug() << "after split groups: " << groups;
  }
}

void separateNode(
    Group& g, std::unordered_map<const VisualNode*, PosInGroups>& node2groupID,
    int i) {
  /// swap the target element with the element pointed at by the splitter
  auto& el_1 = g.items[g.splitter];
  auto& el_2 = g.items[i];
  std::swap(el_1, el_2);
  std::swap(node2groupID[el_1.node].inner_idx,
            node2groupID[el_2.node].inner_idx);

  /// advance the splitter
  ++g.splitter;
}

// returns a pair <i,j> where i is a group index and j -- node's index within
// that group
std::pair<int, int> findNodeInGroups(
    const std::vector<Group>& groups,
    std::unordered_map<const VisualNode*, PosInGroups>& node2groupID,
    const VisualNode* n) {
  // for (auto i = 1u; i < groups.size(); ++i) {
  auto g_idx = node2groupID[n].group_idx;
  auto idx = node2groupID[n].inner_idx;

  return std::make_pair(g_idx, idx);

  auto& group_items = groups[g_idx].items;
  for (auto j = 0u; j < group_items.size(); ++j) {
    auto& e = group_items[j];
    if (e.node == n) return std::make_pair(g_idx, j);
  }
  // }
  abort();
  return std::make_pair(-1, -1);
}
}
//--------------------------------------------
//-------- FINDING IDENTICAL SUBTREES --------
//--------------------------------------------
GroupsOfNodes_t findIdenticalShapes(TreeCanvas* tc,
                                                          const NodeTree& nt) {
  /// ------ 0) group by height ------
  std::vector<Group> groups = detail::groupByHeight(*tc, nt);

  /// ------ assign a group id to each node -------
  std::unordered_map<const VisualNode*, PosInGroups> node2groupID;
  for (auto group_idx = 1u; group_idx < groups.size(); ++group_idx) {
    auto& group = groups[group_idx];

    for (auto i = 0u; i < group.items.size(); ++i) {
      auto* node = group.items[i].node;
      node2groupID[node].group_idx = group_idx;
      node2groupID[node].inner_idx = i;
    }
  }

  /// ------ 2) select the first block (with height 1)
  perfHelper.begin("shapes: minimisation algorithm");
  for (auto group_id = 1u; group_id < groups.size(); ++group_id) {
    auto block = groups[group_id];
    // qDebug() << "groups: " << groups;

    /// ------ 3) traverse 'left' elements of that block:
    /// ------
    ///             and separate it from the group

    std::vector<int> groups_to_split;

    // qDebug() << "left children:";
    for (auto& e : block.items) {
      if (e.alt == 0) {
        /// 3.1) get its parent
        auto& na = nt.getNA();
        auto parent = e.node->getParent(na);
        // std::cerr << parent->debug_id << " ";

        /// 3.2 )find it in groups
        perfHelper.accumulate("shapes: find node in groups");
        auto location = detail::findNodeInGroups(groups, node2groupID, parent);
        perfHelper.end("shapes: find node in groups");
        // std::cerr << "in group: " << location.first << "\n";

        /// group g_idx will potentially need splitting
        auto g_idx = location.first;
        groups_to_split.push_back(g_idx);

        detail::separateNode(groups[g_idx], node2groupID, location.second);
        // qDebug() << "groups: " << groups;
        /// split only affected groups
      }
    }
    // std::cerr << '\n';
    perfHelper.accumulate("shapes: split groups");
    detail::splitGroups(groups, groups_to_split, node2groupID);
    perfHelper.end("shapes: split groups");
    groups_to_split.clear();
    // qDebug() << "groups: " << groups;

    // qDebug() << "right children:";
    for (auto& e : block.items) {
      if (e.alt == 1) {
        /// 3.1) get its parent
        auto& na = nt.getNA();
        auto parent = e.node->getParent(na);
        // std::cerr << parent->debug_id << " ";

        /// 3.2 )find it in groups
        perfHelper.accumulate("shapes: find node in groups");
        auto location = detail::findNodeInGroups(groups, node2groupID, parent);
        perfHelper.end("shapes: find node in groups");

        auto g_idx = location.first;
        groups_to_split.push_back(g_idx);
        // std::cerr << "in group: " << location.first << "\n";
        detail::separateNode(groups[g_idx], node2groupID, location.second);
        // qDebug() << "groups: " << groups;
        /// split only affected groups
      }
    }
    perfHelper.accumulate("shapes: split groups");
    detail::splitGroups(groups, groups_to_split, node2groupID);
    perfHelper.end("shapes: split groups");
  }
/// ----- sort the groups -----
#ifdef MAXIM_DEBUG
  sort(begin(groups), end(groups), [](const Group& lhs, const Group& rhs) {
    if (lhs.items.size() == 0 && rhs.items.size() == 0) {
      return false;
    }
    if (lhs.items.size() == 0) {
      return false;
    }
    if (rhs.items.size() == 0) {
      return true;
    }
    return lhs.items[0].node->debug_id < rhs.items[0].node->debug_id;
  });
#endif

  perfHelper.end();
  // qDebug() << "final groups: " << groups;
  perfHelper.total("shapes: find node in groups");
  perfHelper.total("shapes: split groups");
  perfHelper.total("actual splitting");

  /// convert groups to a vector of vectors of Nodes

  perfHelper.begin("shapes: post-process the results");
  std::vector<std::vector<VisualNode*>> result(groups.size());
  for (auto i = 0u; i < groups.size(); ++i) {
    auto& items = groups[i].items;

    result[i].reserve(items.size());

    // qDebug() << "copying " << items.size() << " elements";
    for (auto j = 0u; j < items.size(); ++j) {
      result[i].push_back(items[j].node);
    }
  }
  perfHelper.end();

  return result;
}
}
}
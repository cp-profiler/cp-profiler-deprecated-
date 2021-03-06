namespace cpprofiler {
namespace analysis {
namespace subtrees {


namespace identical_subtrees_old {

#ifdef MAXIM_DEBUG
static void printGroup(const Group& g) {
  std::stringstream ss;

  for (auto i = 0u; i < g.splitter && i < g.items.size(); ++i) {
    ss << g.items[i].node->debug_id << " ";
  }

  ss << "|";

  for (auto i = g.splitter; i < g.items.size(); ++i) {
    ss << g.items[i].node->debug_id << " ";
  }
  qDebug() << ss.str().c_str();
}

static void printGroups(const std::vector<Group>& groups) {
  for (const auto& g : groups) {
    printGroup(g);
  }
}

static std::stringstream& operator<<(std::stringstream& ss, const ChildInfo& ci) {
  ss << ci.node->debug_id;
  return ss;
}
#endif

static void splitGroups(
    std::vector<Group>& groups,
    const std::vector<int>& g_to_split,
    std::unordered_map<const VisualNode*, PosInGroups>& node2groupID) {

  for (auto g_idx : g_to_split) {

    auto& g = groups[g_idx];
    if (g.splitter == 0 || g.splitter == (int)g.items.size()) {
      g.splitter = 0;
      /// ----- don't need to split the group -----
      continue;
    }
    /// ----- need to split the group -----
    auto it_splitter = begin(g.items);
    std::advance(it_splitter, g.splitter);

    Group new_group2{0, {it_splitter, end(g.items)}};

    g.items.erase(it_splitter, end(g.items));
    g.splitter = 0;

    groups.push_back(new_group2);
    /// ----- change group_id for nodes in the second group -----
    for (auto j = 0u; j < new_group2.items.size(); ++j) {
      auto* node = new_group2.items[j].node;
      node2groupID[node].g_id = groups.size() - 1;
      node2groupID[node].inner_idx = j;
    }



    // for (auto idx = g_idx + 2; idx < groups.size(); ++idx) {
    //   for (auto& ci : groups[idx].items) {
    //     auto* n = ci.node;
    //     node2groupID[n].g_id = idx;
    //   }
    // }
    // groups.push_back(std::move(new_group2));

    // ++i;
    // qDebug() << "after split groups: " << groups;
  }
}

static void separateNode(
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
static std::pair<int, int> findNodeInGroups(
    std::unordered_map<const VisualNode*, PosInGroups>& node2groupID,
    const VisualNode* n) {

  const auto g_idx = node2groupID[n].g_id;
  const auto idx = node2groupID[n].inner_idx;

  return std::make_pair(g_idx, idx);
}


//--------------------------------------------
//-------- FINDING IDENTICAL SUBTREES --------
//--------------------------------------------
// TODO/NOTE(maxim): This algorithm isn't supposed to work
// (new groups pushed to the end), but I've failed
// to find any cases where it produces a wrong result
GroupsOfNodes_t findIdentical(NodeTree& nt, const GroupsOfNodes_t& init_p) {

  /// ------ 0) Initial Partition ------
  std::vector<Group> groups = prepareGroups(init_p, nt);

    /// *** EXPERIMENT (shuffling) ***
  // std::random_shuffle( groups.begin(), groups.end() );
  /// ******************************

  /// ------ 1) assign a group id to each node -------
  std::unordered_map<const VisualNode*, PosInGroups> node2groupID;
  for (auto group_idx = 0u; group_idx < groups.size(); ++group_idx) {
    auto& group = groups[group_idx];

    for (auto i = 0u; i < group.items.size(); ++i) {
      auto* node = group.items[i].node;
      node2groupID[node].g_id = group_idx;
      node2groupID[node].inner_idx = i;
    }
  }

  // printGroups(groups);
  // qDebug() << " ";

  /// ------ 2) select the first block (with height 1)

  for (auto group_id = 0u; group_id < groups.size(); ++group_id) {

    for (auto alt = 0; alt < 2; ++alt) {

      auto& block = groups[group_id];

      std::vector<int> groups_to_split;

      // qDebug() << "left children:";
      for (auto& e : block.items) {
        if (e.alt == alt) {
          /// 3.1) get its parent
          auto& na = nt.getNA();
          auto* parent = e.node->getParent(na);
          // std::cerr << parent->debug_id << " ";

          /// 3.2 )find it in groups

          auto location = findNodeInGroups(node2groupID, parent);

          // std::cerr << parent->debug_id << " in group: " << location.first << "\n";

          /// group g_idx will potentially need splitting
          auto g_idx = location.first;
          groups_to_split.push_back(g_idx); /// NOTE(maxim): has duplicate elements (?)

          separateNode(groups[g_idx], node2groupID, location.second);
          // qDebug() << "node: " << e.node->debug_id;
          // qDebug() << "separate: " << parent->debug_id;
          /// split only affected groups
        }
      }

      // qDebug() << "groups before:";
      // printGroups(groups);
      splitGroups(groups, groups_to_split, node2groupID);
      // qDebug() << "groups after:";
      // printGroups(groups);
      // qDebug() << " ";
      groups_to_split.clear();
      // qDebug() << "groups: " << groups;
    }
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


  /// convert groups to a vector of vectors of Nodes

  std::vector<std::vector<VisualNode*>> result(groups.size());
  for (auto i = 0u; i < groups.size(); ++i) {
    auto& items = groups[i].items;

    result[i].reserve(items.size());

    // qDebug() << "copying " << items.size() << " elements";
    for (auto j = 0u; j < items.size(); ++j) {
      result[i].push_back(items[j].node);
    }
  }

  return result;
}


}}}}

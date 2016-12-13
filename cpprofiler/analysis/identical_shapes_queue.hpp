
#include <set>

namespace cpprofiler {
namespace analysis {
namespace subtrees {
namespace identical_subtrees_queue {


static void splitGroups(
    std::vector<Group>& groups,
    const std::set<int>& g_to_split,
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

    Group new_group{0, {it_splitter, end(g.items)}};

    g.items.erase(it_splitter, end(g.items));
    g.splitter = 0;

    groups.push_back(new_group);
    /// ----- change group_id for nodes in the second group -----
    for (auto j = 0u; j < new_group.items.size(); ++j) {
      auto* node = new_group.items[j].node;
      node2groupID[node].g_id = groups.size() - 1;
      node2groupID[node].inner_idx = j;
    }
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

static std::pair<int, int> findNodeInGroups(
    const std::vector<Group>& groups,
    std::unordered_map<const VisualNode*, PosInGroups>& node2groupID,
    const VisualNode* n) {

  const auto g_idx = node2groupID[n].g_id;
  const auto idx = node2groupID[n].inner_idx;

  return std::make_pair(g_idx, idx);
}


GroupsOfNodes_t findIdentical(NodeTree& nt, const GroupsOfNodes_t& init_p) {

  /// ------ 0) Initial Partition ------
  std::vector<Group> groups = prepareGroups(init_p, nt);

  /// ------ 1) Assign a group id to each node -------
  std::unordered_map<const VisualNode*, PosInGroups> node2groupID;

  for (auto g_idx = 0u; g_idx < groups.size(); ++g_idx) {
    auto& group = groups[g_idx];

    for (auto i = 0u; i < group.items.size(); ++i) {
      auto* node = group.items[i].node;
      node2groupID[node].g_id = g_idx;
      node2groupID[node].inner_idx = i;
    }
  }

  /// ------ 2) select the first block (with height 1)

  for (auto group_id = 0u; group_id < groups.size(); ++group_id) {

    for (auto alt = 0; alt < 2; ++alt) {

      auto& block = groups[group_id];

      std::set<int> groups_to_split;

      for (auto& e : block.items) {
        if (e.alt == alt) {
          /// 3.1) get its parent
          auto& na = nt.getNA();
          auto* parent = e.node->getParent(na);
          // std::cerr << parent->debug_id << " ";

          /// 3.2 )find it in groups

          auto location = findNodeInGroups(groups, node2groupID, parent);
          auto g_idx = location.first;
          groups_to_split.insert(g_idx); /// NOTE(maxim): has duplicate elements (?)

          separateNode(groups[g_idx], node2groupID, location.second);
        }
      }


      splitGroups(groups, groups_to_split, node2groupID);

      groups_to_split.clear();

    }
  }


  GroupsOfNodes_t result(groups.size());

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
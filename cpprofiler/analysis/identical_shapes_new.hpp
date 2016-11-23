
namespace cpprofiler { namespace analysis {


/// Does not work correctly!!!
namespace identical_subtrees_new {

class Group;

struct ChildInfo {
  int alt;
  VisualNode* node;
};

struct PosInGroups2 {
  int inner_idx;
  Group* group;
};

#include "splittable_groups.hpp"



using ChildrenInfoGroups = vector<vector<ChildInfo>>;

namespace detail {

  /// get subtree height, while also populating group_items
  static int getSubtreeHeight(const VisualNode* n, const NodeAllocator& na,
                       ChildrenInfoGroups& groups) {
    int max = 0;

    int kids = n->getNumberOfChildren();

    if (kids == 0) {
      return 1;
    }

    for (int i = 0; i < kids; ++i) {
      auto kid = n->getChild(na, i);
      int h = getSubtreeHeight(kid, na, groups);
      auto& group_items = groups[h - 1];
      group_items.push_back({i, kid});
      if (h > max) {
        max = h;
      }
    }

    return max + 1;
  }


  static ChildrenInfoGroups groupByHeight2(const TreeCanvas& tc, NodeTree& nt) {
    int max_depth = tc.get_stats().maxDepth;

    /// start from 1 for convenience
    ChildrenInfoGroups groups(max_depth);

    auto& na = nt.getNA();
    auto* root = nt.getRoot();

    auto max_depth_ignored = getSubtreeHeight(root, na, groups);
    assert (max_depth == max_depth_ignored);

    /// edge case of a root node
    groups[groups.size() - 1].push_back({-1, root});

    return groups;
  }

  std::pair<int, int> findNodeInGroups2(
      std::unordered_map<const VisualNode*, PosInGroups>& node2groupID,
      const VisualNode* n) {

    const auto g_idx = node2groupID[n].g_id;
    const auto idx = node2groupID[n].inner_idx;

    return std::make_pair(g_idx, idx);
  }

  void separateNode(Group* g,
                    std::unordered_map<const VisualNode*, PosInGroups2>& node2pos,
                    int idx) {
    /// swap the target element with the element pointed at by the splitter
    auto& el_1 = g->at(g->splitter);
    auto& el_2 = g->at(idx);

    // qDebug() << "separate " << el_2.node->debug_id;
    std::swap(el_1, el_2);
    std::swap(node2pos[el_1.node].inner_idx, node2pos[el_2.node].inner_idx);

    /// advance the splitter
    g->increment_splitter();
  }

  void splitGroups2(
      SplittableGroups& sgroups,
      const std::vector<Group*>& g_to_split,
      std::unordered_map<const VisualNode*, PosInGroups2>& node2pos) {

    for (auto i = 0u; i < g_to_split.size(); ++i) {
      auto* g = g_to_split[i];

      if (g->splitter == 0 || g->splitter == g->m_end - g->m_start) {
        g->splitter = 0;
        /// ----- don't need to split the group -----
        continue;
      }
      /// ----- need to split the group -----
      // printGroup(g);
      
      Group* new_group = sgroups.split_group(g);

      /// ----- change group_id for nodes in the second group -----
      for (auto j = 0u; j < new_group->size(); ++j) {
        auto* node = new_group->at(j).node;
        node2pos[node].group = new_group;
        node2pos[node].inner_idx = j;
      }
      // // printGroup(g);
      // // printGroup(new_group2);
      // sgroups.push_back(std::move(new_group2));


      // ++i;
      // qDebug() << "after split groups: " << groups;
    }
  }
}




GroupsOfNodes_t findIdenticalShapes(TreeCanvas& tc, NodeTree& nt) {
  /// ------ 0) group by height ------
  ChildrenInfoGroups groups = detail::groupByHeight2(tc, nt);

  std::unordered_map<const VisualNode*, PosInGroups2> node2pos;
  SplittableGroups sgroups{groups};

  for (auto& group : sgroups) {
    for (auto i = 0u; i < group->size(); ++i) {
      auto& ci = group->at(i);

      node2pos[ci.node] = PosInGroups2{(int)i, group};
    }
  }


  // print_groups(sgroups);
  // print(sgroups);

  for (auto g_idx = 0; g_idx < sgroups.size(); ++g_idx) {

    auto* group = sgroups[g_idx];
    /// separately for each type of child node
    for (auto alt = 0; alt < 2; ++alt) {

      std::vector<Group*> groups_to_split;
      for (auto& ci : *group) {
        if (ci.alt == alt) {
          // /// 3.1) get its parent
          auto& na = nt.getNA();
          auto* parent = ci.node->getParent(na);
          // // std::cerr << parent->debug_id << " ";

          // /// 3.2 )find it in groups

          // qDebug() << "cause node: " << ci.node->debug_id;

          auto& location = node2pos.at(parent);
          auto* p_group = location.group;
          auto p_inner_idx = location.inner_idx;
          // /// group g_idx will potentially need splitting
          groups_to_split.push_back(p_group); /// NOTE(maxim): has duplicate elements

          detail::separateNode(p_group, node2pos, p_inner_idx);
          // // qDebug() << "groups: " << groups;
          // /// split only affected groups
        }

      }


      // print_groups(sgroups);
      perfHelper.accumulate("splitGroups2");
      detail::splitGroups2(sgroups, groups_to_split, node2pos);
      perfHelper.end("splitGroups2");
      // print_groups(sgroups);
    }



  }

  perfHelper.total("splitGroups2");

  // print_groups(sgroups);

  std::vector<std::vector<VisualNode*>> result;

  for (auto& group : sgroups) {

    std::vector<VisualNode*> tmp;
    for (auto& ci : *group) {
      tmp.push_back(ci.node);
    }

    result.push_back(std::move(tmp));
  }


  return result;
}

}
}
}

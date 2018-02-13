
namespace cpprofiler {
namespace analysis {
namespace subtrees {

namespace identical_subtrees_new {

class Group;

struct PosInGroups {
  int inner_idx;
  Group* group;
};

#include "splittable_groups.hpp"

namespace detail {

  void separateNode(Group* g,
                    std::unordered_map<const VisualNode*, PosInGroups>& node2pos,
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

  void splitGroups(
      SplittableGroups& sgroups,
      const std::vector<Group*>& g_to_split,
      std::unordered_map<const VisualNode*, PosInGroups>& node2pos) {

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
      for (auto j = 0; j < new_group->size(); ++j) {
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

static ChildrenInfoGroups prepareGroups(const GroupsOfNodes_t& init_p, const NodeTree& nt) {

  ChildrenInfoGroups groups;

  for (auto& vec : init_p) {
    vector<ChildInfo> g;
    for (auto* n : vec) {
      g.push_back(node2ci(n, nt));
    }
    groups.push_back(g);
  }

  return groups;
}

GroupsOfNodes_t findIdentical(NodeTree& nt, const GroupsOfNodes_t& init_p) {


  // / 0) Start with some initial partition:
  ChildrenInfoGroups groups = prepareGroups(init_p, nt);

  std::unordered_map<const VisualNode*, PosInGroups> node2pos;
  SplittableGroups sgroups{groups};

  for (auto& group : sgroups) {
    for (auto i = 0; i < group->size(); ++i) {
      auto& ci = group->at(i);

      node2pos[ci.node] = PosInGroups{(int)i, group};
    }
  }

  // print_groups(sgroups);
  // print(sgroups);

  for (auto g_idx = 0u; g_idx < sgroups.size(); ++g_idx) {

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

          // NOTE: sometimes p_group->m_start == p_group->m_end (is that a bug?)
          detail::separateNode(p_group, node2pos, p_inner_idx);
          // // qDebug() << "groups: " << groups;
          // /// split only affected groups
        }

      }


      // print_groups(sgroups);
      // perfHelper.accumulate("splitGroups");
      detail::splitGroups(sgroups, groups_to_split, node2pos);
      // perfHelper.end("splitGroups");
      // print(sgroups);
      // print_groups(sgroups);
    }



  }

  // print_groups(sgroups);

  std::vector<std::vector<VisualNode*>> result;

  for (auto& group : sgroups) {

    // qDebug() << "size: " << group->size();

    std::vector<VisualNode*> tmp;
    for (auto& ci : *group) {
      tmp.push_back(ci.node);
    }

    result.push_back(std::move(tmp));
  }

  // for (auto& group : groups) {
  //   std::vector<VisualNode*> tmp;
  //   // qDebug() << "size: " << group.size();
  //   for (auto& ci : group) {
  //       tmp.push_back(ci.node);
  //       // qDebug() << ci.node->debug_id;
  //   }
  //   result.push_back(std::move(tmp));
  // }


  return result;
}

}
}
}
}
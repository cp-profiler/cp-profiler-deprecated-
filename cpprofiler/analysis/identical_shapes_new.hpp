
namespace cpprofiler {
namespace analysis {
namespace subtrees {

namespace identical_subtrees_new {

class Group;

struct ChildInfo {
  int alt;
  VisualNode* node;
};

struct PosInGroups {
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


  static ChildrenInfoGroups groupByHeight(NodeTree& nt) {
    int max_depth = nt.getStatistics().maxDepth;

    ChildrenInfoGroups groups(max_depth);

    auto& na = nt.getNA();
    auto* root = nt.getRoot();

    auto max_depth_ignored = getSubtreeHeight(root, na, groups);
    assert (max_depth == max_depth_ignored);

    /// edge case of a root node
    groups[groups.size() - 1].push_back({-1, root});

    return groups;
  }

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

static int countNodes(VisualNode* node,
                      const NodeAllocator& na,
                      std::unordered_map<int, vector<ChildInfo>>& map) {
    if (node->getNumberOfChildren() == 0) return 1;

    int result = 0;
    for (auto i = 0u; i < node->getNumberOfChildren(); ++i) {
        auto* child = node->getChild(na, i);
        auto count = countNodes(child, na, map);
        result += count;
        map[count].push_back({(int)i, child});
    }

    return result;
}


static ChildrenInfoGroups groupByNoNodes(NodeTree& nt) {

    std::unordered_map<int, vector<ChildInfo>> group_map;

    auto& na = nt.getNA();
    auto* root = nt.getRoot();

    auto count = countNodes(root, na, group_map);
    group_map[count].push_back({-1, root});

    /// convert a map into a vector

    ChildrenInfoGroups result;
    result.reserve(group_map.size());

    for (auto& pair : group_map) {
        result.push_back(std::move(pair.second));
    }

    return result;
}

static ChildrenInfoGroups shapesToGroups(const std::vector<ShapeInfo>& shapes,
                                         const NodeTree& nt) {
  const auto& na = nt.getNA();

  ChildrenInfoGroups result;
  result.reserve(shapes.size());

  for (const auto& s : shapes) {

    vector<ChildInfo> group;
    for (auto* n : s.nodes) {

      /// figure out `alt`: have to check all parent's children
      int alt;
      auto* p = n->getParent(na);

      if (p != nullptr) {

        for (auto i = 0u; i < p->getNumberOfChildren(); ++i) {
          if (n == p->getChild(na, i)) {
            alt = i;
            break;
          }
        }

      } else {
        alt = -1;
      }

      group.push_back({(int)alt, n});
    }

    result.push_back(std::move(group));
  }

  return result;
}

GroupsOfNodes_t findIdentical(NodeTree& nt) {

  // std::vector<ShapeInfo> shapes = runSimilarShapes(nt);

  // ChildrenInfoGroups groups = shapesToGroups(shapes, nt);
  // / 0) Start with some initial partition:
  ChildrenInfoGroups groups = groupByNoNodes(nt); /// slightly faster


  // ChildrenInfoGroups groups = detail::groupByHeight(nt);

  std::unordered_map<const VisualNode*, PosInGroups> node2pos;
  SplittableGroups sgroups{groups};

  for (auto& group : sgroups) {
    for (auto i = 0u; i < group->size(); ++i) {
      auto& ci = group->at(i);

      node2pos[ci.node] = PosInGroups{(int)i, group};
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
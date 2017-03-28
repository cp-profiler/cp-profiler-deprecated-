
#include <sstream>

#include "similar_shapes.hh"

#include <vector>
#include <string>
#include <unordered_map>
#include <regex>

#include "visualnode.hh"
#include "cpprofiler/utils/tree_utils.hh"
#include "execution.hh"
#include "libs/perf_helper.hh"
#include "similar_shape_algorithm.hh"

class NodeAllocator;

namespace cpprofiler {
namespace analysis {
namespace subtrees {

namespace identical_subtrees_new {
  GroupsOfNodes_t findIdentical(NodeTree& nt, const GroupsOfNodes_t& init_p);
}

namespace identical_subtrees_old {
  GroupsOfNodes_t findIdentical(NodeTree& nt, const GroupsOfNodes_t& init_p);
}

namespace identical_subtrees_flat {
  GroupsOfNodes_t findIdentical(NodeTree& nt, const GroupsOfNodes_t& init_p);
}

namespace identical_subtrees_queue {
  GroupsOfNodes_t findIdentical(NodeTree& nt, const GroupsOfNodes_t& init_p);
}

struct ChildInfo {
  int alt;
  VisualNode* node;
};

static ChildInfo node2ci(VisualNode* n, const NodeTree& nt) {
  auto& na = nt.getNA();
  /// figure out `alt`: have to check all parent's children
  auto* p = n->getParent(na);
  int alt;
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

  return {alt, n};
}

using std::vector;
using ChildrenInfoGroups = vector<vector<ChildInfo>>;

class Group {
public:
  int splitter = 0;
  std::vector<ChildInfo> items;
  Group() = default;
  Group(const Group& other) {
    splitter = other.splitter;
    items = other.items;
  }
  Group(int s, const std::vector<ChildInfo>& i) : splitter(s), items(i) {}
};

/// transform initial partition into appropriate format for the algorithm ("flat", "queue", "old")
static vector<Group> prepareGroups(const GroupsOfNodes_t& init_p, const NodeTree& nt) {

  vector<Group> groups;

  for (auto& vec : init_p) {
    Group g;
    for (auto* n : vec) {
      g.items.push_back(node2ci(n, nt));
    }
    groups.push_back(g);
  }

  return groups;
}

QDebug& operator<<(QDebug& os, const Group& g) {
  std::ostringstream oss;
  oss << "[ ";
  for (int i = 0; i < (int)g.items.size(); ++i) {
    if (i == g.splitter) {
      oss << "||";
    }
    auto& info = g.items[i];
    oss << "<" << info.alt << ", " << info.node << "> ";
  }
  if ((int)g.items.size() == g.splitter) {
      oss << "||";
  }
  oss << "]";
  os << oss.str().c_str();
  return os;
}

QDebug& operator<<(QDebug& os, const std::vector<Group>& groups) {
  for (auto i = 1u; i < groups.size(); ++i) {
    os << groups[i] << "\n";
  }
  return os;
}

template<typename T>
static vector<T> flatten(vector<vector<T>>& vecs) {

  vector<T> result;

  for (auto& v : vecs) {
    for (auto& e : v) {
      result.push_back(e);
    }
  }

  return result;
}


/// Groups nodes by height of their underlying subtree
static GroupsOfNodes_t groupByHeight(NodeTree& nt) {

  using T = VisualNode;
  using Fn = std::function<int(const T*, const NodeAllocator&, GroupsOfNodes_t&)>;

  Fn getSubtreeHeight = [&getSubtreeHeight](const T* n,
                            const NodeAllocator& na,
                            GroupsOfNodes_t& groups) -> int {
    int max = 0;

    int kids = n->getNumberOfChildren();

    if (kids == 0) {
      return 1;
    }

    for (int i = 0; i < kids; ++i) {
      auto kid = n->getChild(na, i);
      int h = getSubtreeHeight(kid, na, groups);
      auto& group_items = groups[h - 1];
      group_items.push_back(kid);
      if (h > max) {
        max = h;
      }
    }

    return max + 1;
  };

  int max_depth = utils::calculateMaxDepth(nt);

  GroupsOfNodes_t groups(max_depth);

  auto& na = nt.getNA();
  auto* root = nt.getRoot();

  auto max_depth_ignored = getSubtreeHeight(root, na, groups);
  assert (max_depth == max_depth_ignored);

  /// edge case of a root node
  groups[groups.size() - 1].push_back(root);

  return groups;
}

static GroupsOfNodes_t groupByNoNodes(NodeTree& nt) {

    using T = VisualNode;
    using Fn = std::function<int(T*, const NodeAllocator&, std::unordered_map<int, vector<T*>>&)>;

    Fn countNodes = [&](T* node,
                      const NodeAllocator& na,
                      std::unordered_map<int, vector<T*>>& map) -> int {
      if (node->getNumberOfChildren() == 0) return 1;

      int result = 0;
      for (auto i = 0u; i < node->getNumberOfChildren(); ++i) {
          auto* child = node->getChild(na, i);
          auto count = countNodes(child, na, map);
          result += count;
          map[count].push_back(child);
      }

      return result;
    };

    std::unordered_map<int, vector<T*>> group_map;

    auto& na = nt.getNA();
    auto* root = nt.getRoot();

    auto count = countNodes(root, na, group_map);
    group_map[count].push_back(root);

    /// convert a map into a vector

    GroupsOfNodes_t result;
    result.reserve(group_map.size());

    for (auto& pair : group_map) {
        result.push_back(std::move(pair.second));
    }

    return result;
}

static GroupsOfNodes_t groupByShapes(NodeTree& nt) {

  auto shapesToGroups = [] (const std::vector<ShapeInfo>& shapes,
                                         const NodeTree& nt) {
    const auto& na = nt.getNA();

    GroupsOfNodes_t result;
    result.reserve(shapes.size());

    for (const auto& s : shapes) {
      vector<VisualNode*> group;
      for (auto* n : s.nodes) {
        group.push_back(n);
      }
      result.push_back(std::move(group));
    }
    return result;
  };

  std::vector<ShapeInfo> shapes = runSimilarShapes(nt);
  return shapesToGroups(shapes, nt);
}

struct PosInGroups {
  int g_id; /// Group identifier
  int inner_idx; /// ?
};

std::string extractVar(const std::string& label) {

      auto found = label.find_first_of("!=><?");

      if (found != std::string::npos) {
        return label.substr(0, found);
      }
      return label;
}

static GroupsOfNodes_t groupByLabels(Execution& ex, bool vars_only = false) {

  using std::string;

  auto& nt = ex.nodeTree();

  GroupsOfNodes_t result;

  /// Group by height first

  GroupsOfNodes_t h_part = groupByHeight(nt);

  for (auto& vec : h_part) {

    /// Split vec based on labels

    std::unordered_map<string, vector<VisualNode*>> label_map;

    for (auto* n : vec) {
      string l = ex.getLabel(*n);

      /// truncate l to a contain var name only if `vars_only`
      if (vars_only) { l = extractVar(l); }

      label_map[l].push_back(n);
    }

    /// Add groups into result

    for (auto& pair : label_map) {
      result.push_back(std::move(pair.second));
    }
  }

  return result;

}

GroupsOfNodes_t findIdentical(Execution& ex, LabelOption label_opt = LabelOption::IGNORE) {

  auto& nt = ex.nodeTree();

  /// Initial partition
  GroupsOfNodes_t init_p;

  if (label_opt == LabelOption::FULL) {
    init_p = groupByLabels(ex);
  } else if (label_opt == LabelOption::VARS) {
    init_p = groupByLabels(ex, true);
  } else {
    /// NOTE(maxim): it is also possible to group by height and shapes using
    /// `groupByShapes` (significantly slower) and `groupsByHeight` (a bit slower)
    init_p = groupByNoNodes(nt);
  }

  /// TODO(maxim): figure out which one works best and keep only that one
  // return identical_subtrees_new::findIdentical(nt, init_p);
  // return identical_subtrees_flat::findIdentical(nt, init_p);
  // return identical_subtrees_old::findIdentical(nt, init_p);

  /// TODO(maxim): the below doesn't actually use queue yet
  return identical_subtrees_queue::findIdentical(nt, init_p);
}



}
}
}

#include "identical_shapes_flat.hpp"
#include "identical_shapes_old.hpp"
#include "identical_shapes_new.hpp"
#include "identical_shapes_queue.hpp"
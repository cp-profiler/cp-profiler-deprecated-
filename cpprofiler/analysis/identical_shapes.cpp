
#include <sstream>

#include "similar_shapes.hh"

#include <vector>
#include <unordered_map>

#include "visualnode.hh"
#include "tree_utils.hh"
#include "nodetree.hh"
#include "libs/perf_helper.hh"

class NodeAllocator;

namespace cpprofiler {
namespace analysis {
namespace subtrees {

namespace identical_subtrees_new {
  GroupsOfNodes_t findIdentical(NodeTree& nt);
}

namespace identical_subtrees_old {
  GroupsOfNodes_t findIdentical(NodeTree& nt);
}

namespace identical_subtrees_flat {
  GroupsOfNodes_t findIdentical(NodeTree& nt);
}

namespace identical_subtrees_queue {
  GroupsOfNodes_t findIdentical(NodeTree& nt);
}

struct ChildInfo {
  int alt;
  VisualNode* node;
};

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


/// get subtree height, while also populating group_items
static int getSubtreeHeight(const VisualNode* n, const NodeAllocator& na,
                     std::vector<Group>& groups) {
  int max = 0;

  int kids = n->getNumberOfChildren();

  if (kids == 0) {
    return 1;
  }

  for (int i = 0; i < kids; ++i) {
    auto kid = n->getChild(na, i);
    int h = getSubtreeHeight(kid, na, groups);
    auto& group_items = groups[h - 1].items;
    group_items.push_back({i, kid});
    if (h > max) {
      max = h;
    }
  }

  return max + 1;
}

/// Groups nodes by height of their underlying subtree
/// TODO(maxim): NodeTree should be able to tell its depth
static std::vector<Group> groupByHeight(NodeTree& nt) {
  int max_depth = tree_utils::calculateMaxDepth(nt);

  /// start from 1 for convenience
  std::vector<Group> groups(max_depth);

  auto& na = nt.getNA();
  auto* root = nt.getRoot();

  auto max_depth_ignored = getSubtreeHeight(root, na, groups);
  assert (max_depth == max_depth_ignored);

  /// edge case of a root node
  groups[groups.size() - 1].items.push_back({-1, root});

  return groups;
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

struct PosInGroups {
  int g_id; /// Group identifier
  int inner_idx; /// ?
};



GroupsOfNodes_t findIdentical(NodeTree& nt) {
  // return identical_subtrees_new::findIdenticalShapes(nt);
  // return identical_subtrees_flat::findIdentical(nt);
  // return identical_subtrees_old::findIdentical(nt);
  return identical_subtrees_queue::findIdentical(nt);
}



}
}
}

#include "identical_shapes_flat.hpp"
#include "identical_shapes_old.hpp"
#include "identical_shapes_new.hpp"
#include "identical_shapes_queue.hpp"
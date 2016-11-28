
namespace cpprofiler {
namespace analysis {

using std::vector;
struct ChildInfo {
  int alt;
  VisualNode* node;
};

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


struct PosInGroups {
  int g_id; /// Group identifier
  int inner_idx; /// ?
};



}
}

#include "identical_shapes_flat.hpp"
#include "identical_shapes_old.hpp"
#include "identical_shapes_new.hpp"
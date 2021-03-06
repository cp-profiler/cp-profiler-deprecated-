#include <stack>
#include <functional>
#include "visualnode.hh"
#include "execution.hh"
#include "cpprofiler/utils/tree_utils.hh"

#include "nodecursor.hh"
#include "nodevisitor.hh"
#include "string_utils.hh"

namespace utils {

using std::stack;

void copyTree(VisualNode* node_t, NodeTree& tree_t,
              const VisualNode* node_s, const NodeTree& tree_s) {

  auto& na_t = tree_t.getNA();
  const auto& na_s = tree_s.getNA();

  stack<VisualNode*> stk_t;
  stack<const VisualNode*> stk_s;

  stk_s.push(node_s);
  stk_t.push(node_t);

  while (stk_s.size() > 0) {

      const VisualNode* n = stk_s.top(); stk_s.pop();
      VisualNode*    next = stk_t.top(); stk_t.pop();

      auto kids = n->getNumberOfChildren();
      next->setNumberOfChildren(kids, na_t);

      next->setStatus(n->getStatus());
      next->dirtyUp(na_t);

      for (auto i = 0u; i < kids; ++i) {
          stk_s.push(n->getChild(na_s, i));
          stk_t.push(next->getChild(na_t, i));
      }
  }

}

void addChildren(VisualNode* node, NodeTree& nt, int kids) {

  auto& na = nt.getNA();

  node->setNumberOfChildren(kids, na);
  node->dirtyUp(na);

  auto& stats = nt.getStatistics();
  stats.undetermined += kids;

  int depth = utils::calculateDepth(nt, *node) + 1;
  int new_depth = depth + 1;

  stats.maxDepth = std::max(stats.maxDepth, new_depth);

};

void applyToEachNode(NodeTree& nt, const NodeAction& action) {
  auto& na = nt.getNA();

  for (int i = 0; i < na.size(); ++i) {
    action(na[i]);
  }
}

void applyToEachNode(NodeTree& nt, VisualNode* node, const NodeAction& action) {

  auto& na = nt.getNA();

  std::stack<VisualNode*> stk;

  stk.push(node);

  while(stk.size() > 0) {

    auto* curr_node = stk.top(); stk.pop();
    action(curr_node);

    for (auto i = 0u; i < curr_node->getNumberOfChildren(); ++i) {
      auto child = curr_node->getChild(na, i);
      stk.push(child);
    }
  }
}

void applyToEachNodePO(NodeTree& nt, const NodeAction& action) {

  /// PO-traversal requires two stacks
  std::stack<VisualNode*> stk_1;
  std::stack<VisualNode*> stk_2;

  auto* root = nt.getRoot();
  auto& na = nt.getNA();

  stk_1.push(root);

  while (stk_1.size() > 0) {
    auto* node = stk_1.top(); stk_1.pop();

    stk_2.push(node);

    for (auto i = 0u; i < node->getNumberOfChildren(); ++i) {
      auto kid = node->getChild(na, i);
      stk_1.push(kid);
    }
  }

  while (stk_2.size() > 0) {
    auto* node = stk_2.top(); stk_2.pop();

    action(node);
  }

}

bool compareSubtrees(const NodeTree& nt, const VisualNode& root1,
                     const VisualNode& root2) {
  // compare roots
  bool equal = compareNodes(root1, root2);
  if (!equal) return false;

  // if nodes have children, compare them recursively:
  for (auto i = 0u; i < root1.getNumberOfChildren(); ++i) {
    auto new_root_1 = nt.getChild(root1, i);
    auto new_root_2 = nt.getChild(root2, i);

    bool equal = compareSubtrees(nt, *new_root_1, *new_root_2);
    if (!equal) return false;
  }

  return true;
}

/// Distance to the root
int calculateDepth(const NodeTree& nt, const VisualNode& node) {
  int count = 0;

  auto it = &node;
  auto& na = nt.getNA();

  while ( (it = it->getParent(na)) ) { count++; }

  return count;
}

/// Distance to the lowest leaf
static int calcDepth(const NodeAllocator& na, const VisualNode* n) {
  auto kids = n->getNumberOfChildren();
  if (kids == 0) { return 1; }

  int max = 0;
  for (auto i = 0u; i < kids; ++i) {
    int cur_depth = calcDepth(na, n->getChild(na, i)) + 1;
    if (cur_depth > max) { max = cur_depth; }
  }

  return max;
}

int calculateMaxDepth(const NodeTree& nt) {

  const auto& na = nt.getNA();
  const auto* root = nt.getRoot();

  return calcDepth(na, root);
}

int calculateHeight(const NodeTree& nt, const VisualNode& n) {
  const auto& na = nt.getNA();
  return calcDepth(na, &n);
}

void highlightSubtrees(NodeTree& nt, const std::vector<VisualNode*>& nodes,
                       bool hideNotHighlighted) {

  QMutexLocker lock(&nt.getTreeMutex());

  auto& na = nt.getNA();
  auto* root = nt.getRoot();

  root->unhideAll(na);

  {
    QMutexLocker lock(&nt.getLayoutMutex());
    root->layout(na);
  }

  // unhighlight all
  applyToEachNode(nt, [](VisualNode* n) {
    n->setHighlighted(false);
  });

  for (auto node : nodes) {
    node->setHighlighted(true);
  }

  if (hideNotHighlighted) {
    HideNotHighlightedCursor hnhc(root, na);
    PostorderNodeVisitor<HideNotHighlightedCursor>(hnhc).run();
  }

  nt.treeModified();
}

Statistics gatherNodeStats(NodeTree& nt) {
  Statistics stats;

  auto* root = nt.getRoot();

  applyToEachNode(nt, root, [&stats](VisualNode* n) {
    switch (n->getStatus()) {
      case SOLVED:
        stats.solutions += 1; break;
      case FAILED:
        stats.failures += 1; break;
      case BRANCH:
        stats.choices += 1; break;
      case UNDETERMINED:
        stats.undetermined += 1; break;
      default:
        break;
    }
  });

  stats.maxDepth = calculateMaxDepth(nt);

  return stats;
}

std::string compareDomains(const Execution& ex, const VisualNode& lhs,
                    const VisualNode& rhs) {

  std::stringstream result;

  auto l_info = ex.getInfo(lhs);
  auto r_info = ex.getInfo(rhs);

  if (l_info == nullptr || r_info == nullptr) return "";

  auto l_lines = split(*l_info, '\n');
  auto r_lines = split(*r_info, '\n');

  if (l_lines.size() != r_lines.size()) return "";

  for (auto i = 0u; i < l_lines.size(); i++) {
    /// End of domain-specific info
    if (l_lines[i].substr(0, 16) == "full domain size") break;
    if (l_lines[i].substr(0, 16) == "domain reduction") break;

    if (l_lines[i] != r_lines[i]) {
      if (l_lines[i].substr(0, 12) == "X_INTRODUCED") continue;
      result << l_lines[i] << "| " << r_lines[i] << "\n";
    }

  }

  return result.str();

}


void unhideFromNodeToRoot(NodeTree& nt, VisualNode& n) {
  QMutexLocker lock(&nt.getTreeMutex());

  auto root = nt.getRoot();

  auto cur_node = &n;

  /// NOTE(maxim): assuming n belongs to nt
  while (cur_node != root) {

    /// TODO(maxim): should stop when cur_node is visible (different from isHidded())

      cur_node->setHidden(false);
      cur_node = cur_node->getParent(nt.getNA());
  }

}


}

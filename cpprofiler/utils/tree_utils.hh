#ifndef CPPROFILER_TREE_UTILS
#define CPPROFILER_TREE_UTILS

#include <functional>

class VisualNode;
class NodeTree;
class Execution;

namespace utils {

using NodeAction = std::function<void(VisualNode*)>;
/// TODO(maxim): update stats
/// Copy tree structure and node status only
void copyTree(VisualNode* node_t, NodeTree& nt_t,
              const VisualNode* node_s, const NodeTree& nt_s);

/// Add `kids` to `node` updating stats, but not checking
/// if the node already has children
void addChildren(VisualNode* node, NodeTree& nt, int kids);

/// Apply `action` to each node (unspecified order)
void applyToEachNode(NodeTree& nt, const NodeAction& action);

/// Apply `action` to each node (post-order traversal)
void applyToEachNodePO(NodeTree& nt, const NodeAction& action);

/// Apply `action` to each node under (including) `node` (pre-order)
void applyToEachNode(NodeTree& nt, VisualNode* node, const NodeAction& action);

/// Compare subtrees represented by root1 and root2 (returns true if equal)
bool compareSubtrees(const NodeTree& nt, const VisualNode& root1,
                     const VisualNode& root2);

/// Hide all subtrees except for those represented by `nodes`
void highlightSubtrees(NodeTree& nt, const std::vector<VisualNode*>& nodes,
    bool hideNotHighlighted = true);

/// The distance between current node to the root
int calculateDepth(const NodeTree& nt, const VisualNode& node);

int calculateHeight(const NodeTree& nt, const VisualNode& n);

int calculateMaxDepth(const NodeTree& nt);

void unhideFromNodeToRoot(NodeTree& nt, VisualNode& n);

Statistics gatherNodeStats(NodeTree& nt);

std::string compareDomains(const Execution& ex, const VisualNode& lhs,
                    const VisualNode& rhs);
}

#endif
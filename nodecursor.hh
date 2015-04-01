#ifndef GECODE_GIST_NODECURSOR_HH
#define GECODE_GIST_NODECURSOR_HH

#include "visualnode.hh"
#include "data.hh"

#include <vector>

/// \brief A cursor that can be run over a tree
template<class Node>
class NodeCursor {
private:
    /// The node where the iteration starts
    Node* _startNode;
    /// The current node
    Node* _node;
    /// The current alternative
    unsigned int _alternative;
protected:
    /// The node allocator
    const typename Node::NodeAllocator& na;
    /// Set current node to \a n
    void node(Node* n);
    /// Return start node
    Node* startNode(void);
public:
    /// Construct cursor, initially set to \a theNode
    NodeCursor(Node* theNode, const typename Node::NodeAllocator& na);
    /// Return current node
    Node* node(void);
    /// Return current alternative
    unsigned int alternative(void);
    /// Set current alternative
    void alternative(unsigned int a);

    /// \name Cursor interface
    //@{
    /// Test if the cursor may move to the parent node
    bool mayMoveUpwards(void);
    /// Move cursor to the parent node
    void moveUpwards(void);
    /// Test if cursor may move to the first child node
    bool mayMoveDownwards(void);
    /// Move cursor to the first child node
    void moveDownwards(void);
    /// Test if cursor may move to the first sibling
    bool mayMoveSidewards(void);
    /// Move cursor to the first sibling
    void moveSidewards(void);
    //@}
};

/// \brief A cursor that marks failed subtrees as hidden
class HideFailedCursor : public NodeCursor<VisualNode> {
private:
    bool onlyDirty;
public:
    /// Constructor
    HideFailedCursor(VisualNode* theNode,
                     const VisualNode::NodeAllocator& na,
                     bool onlyDirtyNodes);
    /// \name Cursor interface
    //@{
    /// Test if the cursor may move to the first child node
    bool mayMoveDownwards(void);
    /// Process node
    void processCurrentNode(void);
    //@}
};

/// \brief A cursor that marks all nodes in the tree as not hidden
class UnhideAllCursor : public NodeCursor<VisualNode> {
public:
    /// Constructor
    UnhideAllCursor(VisualNode* theNode,
                    const VisualNode::NodeAllocator& na);
    /// \name Cursor interface
    //@{
    /// Process node
    void processCurrentNode(void);
    //@}
};

/// \brief A cursor that marks all nodes in the tree as not stopping
class UnstopAllCursor : public NodeCursor<VisualNode> {
public:
    /// Constructor
    UnstopAllCursor(VisualNode* theNode,
                    const VisualNode::NodeAllocator& na);
    /// \name Cursor interface
    //@{
    /// Process node
    void processCurrentNode(void);
    //@}
};

/// \brief A cursor that finds the next solution
class NextSolCursor : public NodeCursor<VisualNode> {
private:
    /// Whether to search backwards
    bool back;
    /// Whether the current node is not a solution
    bool notOnSol(void);
public:
    /// Constructor
    NextSolCursor(VisualNode* theNode, bool backwards,
                  const VisualNode::NodeAllocator& na);
    /// \name Cursor interface
    //@{
    /// Do nothing
    void processCurrentNode(void);
    /// Test if the cursor may move to the parent node
    bool mayMoveUpwards(void);
    /// Test if cursor may move to the first child node
    bool mayMoveDownwards(void);
    /// Move cursor to the first child node
    void moveDownwards(void);
    /// Test if cursor may move to the first sibling
    bool mayMoveSidewards(void);
    /// Move cursor to the first sibling
    void moveSidewards(void);
    //@}
};

/// \brief A cursor that collects statistics
class StatCursor : public NodeCursor<VisualNode> {
private:
    /// Current depth
    int curDepth;
public:
    /// Depth of the search tree
    int depth;
    /// Number of failed nodes
    int failed;
    /// Number of solved nodes
    int solved;
    /// Number of choice nodes
    int choice;
    /// Number of open nodes
    int open;

    /// Constructor
    StatCursor(VisualNode* theNode,
               const VisualNode::NodeAllocator& na);
    
    /// \name Cursor interface
    //@{
    /// Collect statistics
    void processCurrentNode(void);
    /// Move cursor to the first child node
    void moveDownwards(void);
    /// Move cursor to the parent node
    void moveUpwards(void);
    //@}
    
};


class AnalyzeCursor : public NodeCursor<VisualNode> {
protected:
  QHash<VisualNode*,int> nSols;
public:
  //Constructor
  AnalyzeCursor(VisualNode* root, const VisualNode::NodeAllocator& na, TreeCanvas* tc);
  //Add node to the map
  void processCurrentNode(void);
private:
  TreeCanvas* _tc;
};

class HighlightCursor : public NodeCursor<VisualNode> {
public:
  // Constructor
  HighlightCursor(VisualNode* startNode, const VisualNode::NodeAllocator& na);
  // Hightlight all the nodes below
  void processCurrentNode(void);

};

class UnhighlightCursor : public NodeCursor<VisualNode> {
public:
  // Constructor
  UnhighlightCursor(VisualNode* root, const VisualNode::NodeAllocator& na);
  // Unhightlight all the nodes below
  void processCurrentNode(void);

};

class CountSolvedCursor : public NodeCursor<VisualNode> {
public:
  // Constructor
  CountSolvedCursor(VisualNode* startNode, const VisualNode::NodeAllocator& na, int &count);
  // Count solved leaves and store the nubmer in count variable
  void processCurrentNode(void);
private:
  int &_count;

};

/// Hide subtrees that are not highlighted
class HideNotHighlightedCursor : public NodeCursor<VisualNode> {
protected:
  QHash<VisualNode*,bool> onHighlightPath;
public:
  /// Constructor
  HideNotHighlightedCursor(VisualNode* startNode, const VisualNode::NodeAllocator& na);
  /// Process node
  void processCurrentNode(void);
  /// Test if cursor may move to the first child node
  bool mayMoveDownwards(void);
};


/// \brief A cursor that labels branches
class BranchLabelCursor : public NodeCursor<VisualNode> {
private:
    /// The node allocator
    VisualNode::NodeAllocator& _na;
    /// Current TreeCanvas instance (extract labels from data) 
    TreeCanvas& _tc;
    /// Whether to clear labels
    bool _clear;
public:
    /// Constructor
    BranchLabelCursor(VisualNode* theNode, bool clear,
                      VisualNode::NodeAllocator& na, TreeCanvas& tc);
    /// \name Cursor interface
    //@{
    void processCurrentNode(void);
    //@}
};

/// \brief A cursor that frees all memory
class DisposeCursor : public NodeCursor<VisualNode> {
public:
    /// Constructor
    DisposeCursor(VisualNode* theNode,
                  const VisualNode::NodeAllocator& na);
    
    /// \name Cursor interface
    //@{
    /// Dispose node
    void processCurrentNode(void);
    //@}
    
};

class SubtreeCountCursor : public NodeCursor<VisualNode> {
public:
  std::vector<int> stack;
  int threshold;
  SubtreeCountCursor(VisualNode *theNode,
                     int _threshold,
                     const VisualNode::NodeAllocator& na)
    : NodeCursor<VisualNode>(theNode, na),
      threshold(_threshold) {
    stack.push_back(0);
  }
  void processCurrentNode(void) {
    stack.back()++;
    int x = stack.back();
    VisualNode* n = node();
    // A threshold of zero means turn this stuff off.
    if (threshold == 0) {
      n->setSubtreeSizeUnknown();
      n->setHidden(false);
      n->setChildrenLayoutDone(false);
    } else if (x <= threshold) {
      n->setHidden(true);
      n->setChildrenLayoutDone(true);
      if (x < threshold/4) {
        n->setSubtreeSize(0);
      } else if (x < threshold/2) {
        n->setSubtreeSize(1);
      } else if (x < 3*threshold/4) {
        n->setSubtreeSize(2);
      } else {
        n->setSubtreeSize(3);
      }
    } else {
      n->setHidden(false);
      n->setChildrenLayoutDone(false);
    }
    n->dirtyUp(na);
  }
  void moveSidewards(void) {
    NodeCursor<VisualNode>::moveSidewards();
    int x = stack.back();
    stack.pop_back();
    stack.back() += x;
    stack.push_back(0);
  }
  void moveUpwards(void) {
    NodeCursor<VisualNode>::moveUpwards();
    int x = stack.back();
    stack.pop_back();
    stack.back() += x;
  }
  void moveDownwards(void) {
    NodeCursor<VisualNode>::moveDownwards();
    stack.push_back(0);
  }
};

#include "nodecursor.hpp"

#endif

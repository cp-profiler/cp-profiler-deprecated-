/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Guido Tack, 2006
 *
 *  Last modified:
 *     $Date$ by $Author$
 *     $Revision$
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef GECODE_GIST_NODECURSOR_HH
#define GECODE_GIST_NODECURSOR_HH

#include "visualnode.hh"
#include "data.hh"
#include "treecanvas.hh"

#include <vector>
#include <QTextStream>

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
    const NodeAllocator& na;
    /// Set current node to \a n
    void node(Node* n);
    /// Return start node
    Node* startNode(void);
public:
    /// Construct cursor, initially set to \a theNode
    NodeCursor(Node* theNode, const NodeAllocator& na);
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
                     const NodeAllocator& na,
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
                    const NodeAllocator& na);
    /// \name Cursor interface
    //@{
    /// Process node
    void processCurrentNode(void);
    //@}
};

/// \brief A cursor that marks all nodes in the tree as not selected
class UnselectAllCursor : public NodeCursor<VisualNode> {
public:
    /// Constructor
    UnselectAllCursor(VisualNode* theNode,
                      const NodeAllocator& na);
    /// \name Cursor interface
    //@{
    /// Process node
    void processCurrentNode(void);
    //@}
};

/// \brief A cursor that marks ancestor nodes in the tree as not hidden
class UnhideAncestorsCursor : public NodeCursor<VisualNode> {
public:
    /// Constructor
    UnhideAncestorsCursor(VisualNode* theNode,
                    const NodeAllocator& na);
    /// \name Cursor interface
    //@{
    /// Test if the cursor may move to the parent node
    bool mayMoveUpwards(void);
    /// Process node
    void processCurrentNode(void);
    //@}
};

/// \brief A cursor that marks all nodes in the tree as hidden
class HideAllCursor : public NodeCursor<VisualNode> {
public:
    /// Constructor
    HideAllCursor(VisualNode* theNode,
                    const NodeAllocator& na);
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
                    const NodeAllocator& na);
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
                  const NodeAllocator& na);
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

/// \brief A cursor that finds the next leaf
class NextLeafCursor : public NodeCursor<VisualNode> {
private:
    /// Whether to search backwards
    bool back;
    /// Whether the current node is not a leaf
    bool notOnLeaf(void);
public:
    /// Constructor
    NextLeafCursor(VisualNode* theNode, bool backwards,
                   const NodeAllocator& na);
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

/// \brief A cursor that finds the next pentagon
class NextPentagonCursor : public NodeCursor<VisualNode> {
private:
    /// Whether to search backwards
    bool back;
    /// Whether the current node is not a pentagon
    bool notOnPentagon(void);
public:
    /// Constructor
    NextPentagonCursor(VisualNode* theNode, bool backwards,
                  const NodeAllocator& na);
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
               const NodeAllocator& na);

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

namespace cpprofiler { namespace analysis {
  class SimilarShapesWindow;
}}


class SimilarShapesCursor : public NodeCursor<VisualNode> {
protected:
  QHash<VisualNode*,int> nSols;
public:
  //Constructor
  SimilarShapesCursor(VisualNode* root,
                      const NodeAllocator& na,
                      cpprofiler::analysis::SimilarShapesWindow& ssw);
  //Add node to the map
  void processCurrentNode(void);
private:
  cpprofiler::analysis::SimilarShapesWindow& m_ssWindow;
};

class HighlightCursor : public NodeCursor<VisualNode> {
public:
  // Constructor
  HighlightCursor(VisualNode* startNode, const NodeAllocator& na);
  // Highlight all the nodes below
  void processCurrentNode(void);

};

class UnhighlightCursor : public NodeCursor<VisualNode> {
public:
  // Constructor
  UnhighlightCursor(VisualNode* root, const NodeAllocator& na);
  // Unhighlight all the nodes below
  void processCurrentNode(void);

};

class CountSolvedCursor : public NodeCursor<VisualNode> {
public:
  // Constructor
  CountSolvedCursor(VisualNode* startNode, const NodeAllocator& na, int &count);
  // Count solved leaves and store the nubmer in count variable
  void processCurrentNode(void);
private:
  int &_count;

};

class GetIndexesCursor : public NodeCursor<VisualNode> {
public:
  // Constructor
  GetIndexesCursor(VisualNode* startNode, const NodeAllocator& na,
    std::vector<int>& node_gids);
  // Populate node_gids vector with gid of nodes
  void processCurrentNode(void);
private:
  const NodeAllocator& _na;
  std::vector<int>& _node_gids;

};

/// Hide subtrees that are not highlighted
class HideNotHighlightedCursor : public NodeCursor<VisualNode> {
protected:
  QHash<VisualNode*,bool> onHighlightPath;
public:
  /// Constructor
  HideNotHighlightedCursor(VisualNode* startNode, const NodeAllocator& na);
  /// Process node
  void processCurrentNode(void);
  /// Test if cursor may move to the first child node
  bool mayMoveDownwards(void);
};


/// \brief A cursor that labels branches
class BranchLabelCursor : public NodeCursor<VisualNode> {
private:
    /// The node allocator
    NodeAllocator& _na;
    /// Current TreeCanvas instance (extract labels from data)
    TreeCanvas& _tc;
    /// Whether to clear labels
    bool _clear;
public:
    /// Constructor
    BranchLabelCursor(VisualNode* theNode, bool clear,
                      NodeAllocator& na, TreeCanvas& tc);
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
                  const NodeAllocator& na);

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
                     const NodeAllocator& na);
  void processCurrentNode(void);
  void moveSidewards(void);
  void moveUpwards(void);
  void moveDownwards(void);
};

class SearchLogCursor : public NodeCursor<VisualNode> {
private:
    QTextStream& _out;
    /// The node allocator
    const NodeAllocator& _na;
    /// TODO(maxim): should have a reference to the execution here
    const Execution& _execution;
public:
    SearchLogCursor(VisualNode *theNode,
                    QTextStream& outputStream,
                    const NodeAllocator& na,
                    const Execution& execution);
    void processCurrentNode(void);
};

#include "nodecursor.hpp"

#endif

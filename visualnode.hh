/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Guido Tack, 2007
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

#ifndef VISUALNODE_HH
#define VISUALNODE_HH

#include "spacenode.hh"
//#include "treecanvas.hh"
#include <string>

class Data;
//class TreeCanvas;

/// \brief %Layout parameters
class Layout {
public:
  static constexpr int dist_y = 38;
  static constexpr int extent = 20;
  static constexpr int minimalSeparation = 10;
};

/// \brief Bounding box
class BoundingBox {
public:
  /// Left coordinate
  int left;
  /// Right coordinate
  int right;
  /// Default constructor
  BoundingBox(void) {}
};

/// \brief %Extent representing shape of a tree at one depth level
class Extent {
public:
  /// Left extent
  int l;
  /// Right extent
  int r;
  /// Default constructor
  Extent(void);
  /// Construct with \a l0 and \a r0
  Extent(int l0, int r0);
  /// Construct with width \a width
  Extent(int width);

  /// Extend extent by \a deltaL and \a deltaR
  void extend(int deltaL, int deltaR);
  /// Move extent by \a delta
  void move(int delta);
};

/// \brief The shape of a subtree
class Shape {
private:
  /// The depth of this shape
  int _depth;
  /// The bounding box of this shape
  BoundingBox bb;
  /// The shape is an array of extents, one for each depth level
  Extent shape[1];
  /// Copy construtor
  Shape(const Shape&);
  /// Assignment operator
  Shape& operator =(const Shape&);
  /// Constructor
  Shape(void);
public:
  /// Construct shape of depth \a d
  static Shape* allocate(int d);
  /// Destruct
  static void deallocate(Shape*);
  /// Copy \a s
  static Shape* copy(const Shape* s);

  /// Static shape for leaf nodes
  static Shape* leaf;
  /// Static shape for hidden nodes
  static Shape* hidden;

  /// Return depth of the shape
  int depth(void) const;
  /// Set depth of the shape to \a d (must be smaller than original depth)
  void setDepth(int d);
  /// Compute bounding box
  void computeBoundingBox(void);
  /// Return extent at depth \a i
  const Extent& operator [](int i) const;
  /// Return extent at depth \a i
  Extent& operator [](int i);
  /// Return if extent exists at \a depth, if yes return it in \a extent
  bool getExtentAtDepth(int depth, Extent& extent);
  /// Return bounding box
  const BoundingBox& getBoundingBox(void) const;
};

/// \brief %Node class that supports visual layout
class VisualNode : public SpaceNode {

    friend class Data;

protected:
  /// Flags for VisualNodes
  enum VisualNodeFlags {
    DIRTY = SpaceNode::LASTFLAG+1,
    CHILDRENLAYOUTDONE,
    HIDDEN,
    MARKED,
    ONPATH,
    HIGHLIGHTED,
    BOOKMARKED,
    SELECTED, // selected from a pixel tree (TODO(maxim): decide if still needed)
    HOVEREDOVER, // highlighted by hovering over on a pixel tree
    SUBTREESIZE,
    SUBTREESIZE2, // reserve this bit for subtree size
    SUBTREESIZE3,  // reserve this bit for subtree size
  };

  /// Relative offset from the parent node
  int offset;
  /// Shape of this node
  Shape* shape;

  /// Check if the \a x at depth \a depth lies in this subtree
  bool containsCoordinateAtDepth(int x, int depth);
public:
  /// Construct with parent \a p
  VisualNode(int p);
  /// Constructor for root node \a db_id
  VisualNode();
  /// Returns false if any of the nodes in ancestry are hidden
  bool isNodeVisible(const NodeAllocator& na) const;
  /// Return if node is hidden
  bool isHidden(void) const;
  /// Set hidden state to \a h
  void setHidden(bool h);
  /// Set stop state to \a h
  void setStop(bool h);
  /// Mark all nodes up the path to the parent as dirty
  void dirtyUp(const NodeAllocator& na);
  /// Compute layout for the subtree of this node
  void layout(const NodeAllocator& na);
  /// Return offset off this node from its parent
  int getOffset(void);
  /// Set offset of this node, relative to its parent
  void setOffset(int n);
  /// Return whether node is marked as dirty
  bool isDirty(void);
  /// Mark node as dirty
  void setDirty(bool d);
  /// Return whether the layout of the node's children has been completed
  bool childrenLayoutIsDone(void);
  /// Mark node whether the layout of the node's children has been completed
  void setChildrenLayoutDone(bool d);
  /// Return whether node is marked
  bool isMarked(void);
  /// Set mark of this node
  void setMarked(bool m);
  /// Return whether node is selected
  bool isSelected(void);
  /// Set selected flag of this node
  void setSelected(bool m);
  /// Return whether node is hovered over
  bool isHovered(void);
  /// Set hovered over flag of this node
  void setHovered(bool m);
  /// Return whether node is bookmarked
  bool isBookmarked(void);
  /// Set bookmark of this node
  void setBookmarked(bool m);
  /// Return whether node is highlighted
  bool isHighlighted(void);
  /// Highlight the node for similar shapes visualization
  void setHighlighted(bool m);
  /// Set all nodes from the node to the root to be on the path
  void pathUp(const NodeAllocator& na);
  /// Set all nodes from the node to the root not to be on the path
  void unPathUp(const NodeAllocator& na);
  /// Return whether node is on the path
  bool isOnPath(void);
  /// Return the alternative of the child that is on the path (-1 if none)
  int getPathAlternative(const NodeAllocator& na);
  /// Set whether node is on the path
  void setOnPath(bool onPath0);
  /// Set the subtree size to unknown
  void setSubtreeSizeUnknown(void);
  /// Set the subtree size to a specific category
  void setSubtreeSize(int size);
  /// Get the subtree size (-1 if unknown)
  int getSubtreeSize(void);

  /// Toggle whether this node is hidden
  void toggleHidden(const NodeAllocator& na);
  /// Hide all failed subtrees of this node
  void hideFailed(const NodeAllocator& na, bool onlyDirty=false);
  /// Hide subtrees by their size
  void hideSize(int threshold, const NodeAllocator& na);
  /// Unhide all nodes in the subtree of this node
  void unhideAll(const NodeAllocator& na);
  /// Do not stop at this node
  void toggleStop(const NodeAllocator& na);
  /// Do not stop at any stop node in the subtree of this node
  void unstopAll(const NodeAllocator& na);

  /// Return the shape of this node
  Shape* getShape(void);
  /// Set the shape of this node
  void setShape(Shape* s);
  /// Compute the shape according to the shapes of the children
  void computeShape(const NodeAllocator& na);
  /// Return the bounding box
  BoundingBox getBoundingBox(void);
  /// Signal that the status has changed
  void changedStatus(const NodeAllocator& na);
  /// Find a node in this subtree at coordinates \a x, \a y
  VisualNode* findNode(const NodeAllocator& na, int x, int y);

  /// Create or clear branch labels in subtree
  void labelBranches(NodeAllocator& na, TreeCanvas& tc);
  /// Create or clear branch labels on path to root
  void labelPath(NodeAllocator& na, TreeCanvas& tc);
  /// Return string that describes the branch
  std::string getBranchLabel(NodeAllocator& na, VisualNode* p, int alt);

  /// Return string that is used as a tool tip
  std::string toolTip(NodeAllocator& na);

  /// Free allocated memory
  void dispose(void);
};


/// TODO(maxim): move anything to do with labels out
class NodeAllocator {
private:

  std::vector<VisualNode*> nodes;

  /// Hash table mapping nodes to label text
  QHash<VisualNode*, QString> labels;
public:
  NodeAllocator();
  ~NodeAllocator();
  NodeAllocator(const NodeAllocator&) = delete;
  NodeAllocator& operator=(const NodeAllocator&) = delete;
  /// Allocate new node with parent \a p and database id
  int allocate(int p);
  /// Allocate new root node
  int allocateRoot(void);
  /// Return node for index \a i
  VisualNode* operator [](int i) const;
  /// Return branching label flag
  bool showLabels(void) const;
  /// Set branching label flag
  void showLabels(bool b);
  /// Return whether node \a n has a label
  bool hasLabel(VisualNode* n) const;
  /// Set label of node \a n to \a l
  void setLabel(VisualNode* n, const QString& l);
  /// Remove label of node \a n
  void clearLabel(VisualNode* n);
  /// Get label of node \a n
  /// Note(maxim): did I add this?
  QString getLabel(VisualNode* n) const;
  /// returns the total number of nodes allocated
  int size() const;

};

#include "node.hpp"
#include "spacenode.hpp"
#include "visualnode.hpp"

#endif // VISUALNODE_HH

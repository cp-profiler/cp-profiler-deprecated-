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
  static const int dist_y = 38;
  static const int extent = 20;
  static const int minimalSeparation = 10;
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
    DIRTY = SpaceNode::LASTBIT+1,
    CHILDRENLAYOUTDONE,
    HIDDEN,
    MARKED,
    ONPATH,
    HIGHLIGHTED,
    BOOKMARKED
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
  VisualNode(bool);

  /// Return if node is hidden
  bool isHidden(void);
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

  /// Toggle whether this node is hidden
  void toggleHidden(const NodeAllocator& na);
  /// Hide all failed subtrees of this node
  void hideFailed(const NodeAllocator& na, bool onlyDirty=false);
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
  void labelPath(NodeAllocator& na);
  /// Return string that describes the branch
  std::string getBranchLabel(NodeAllocator& na, VisualNode* p, int alt);

  /// Return string that is used as a tool tip
  std::string toolTip(NodeAllocator& na);

  /// Free allocated memory
  void dispose(void);
};

#include "node.hpp"
#include "spacenode.hpp"
#include "visualnode.hpp"

#endif // VISUALNODE_HH

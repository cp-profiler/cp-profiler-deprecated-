#ifndef NODE_HH
#define NODE_HH

#include <cassert>
#include <QHash>
#include <QString>

//#include "data.hh"

class VisualNode;

#include "heap.hpp"
#define GECODE_NEVER assert(false)

/// Node allocator
template<class T>
class NodeAllocatorBase {
private:
  /// Size of each block of nodes
  static const int NodeBlockSize = 1<<14;
  /// Blocks of nodes
  class Block {
  public:
    /// The actual nodes
    T b[NodeBlockSize];
    /// The index of the best known previous solution for each node
    int best[NodeBlockSize];
  };
  /// Array of blocks
  Block** b;
  /// Size of the array
  int n;
  /// Current block number
  int cur_b;
  /// Current node number in current block
  int cur_t;
  /// Allocate new block, potentially reallocate block array
  void allocate(void);
  /// Flag whether search uses branch-and-bound
  bool _bab;
  /// Hash table mapping nodes to label text
  QHash<T*,QString> labels;
public:
  /// Constructor
  NodeAllocatorBase(bool bab);
  /// Destructor
  ~NodeAllocatorBase(void);
  /// Allocate new node with parent \a p and database id
  int allocate(int p, int db_id);
  /// Allocate new root node
  int allocateRoot(int db_id);
  /// Return node for index \a i
  T* operator [](int i) const;
  /// Return index of best node before \a i
  T* best(int i) const;
  /// Set index of best node before \a i to \a b
  void setBest(int i, int b);
  /// Return branch-and-bound flag
  bool bab(void) const;
  /// Return branching label flag
  bool showLabels(void) const;
  /// Set branching label flag
  void showLabels(bool b);
  /// Return whether node \a n has a label
  bool hasLabel(T* n) const;
  /// Set label of node \a n to \a l
  void setLabel(T* n, const QString& l);
  /// Remove label of node \a n
  void clearLabel(T* n);
  /// Get label of node \a n
  QString getLabel(T* n) const;
};

/// \brief Base class for nodes of the search tree
class Node {
private:
  /// Tags that are used to encode the number of children
  enum {
    UNDET, //< Number of children not determined
    LEAF,  //< Leaf node
    TWO_CHILDREN, //< Node with at most two children
    MORE_CHILDREN //< Node with more than two children
  };

  /// The children, or in case there are at most two, the first child
  void* childrenOrFirstChild;

  /// The parent of this node, or NULL for the root
  int parent;

  /// Read the tag of childrenOrFirstChild
  unsigned int getTag(void) const;
  /// Set the tag of childrenOrFirstChild
  void setTag(unsigned int tag);
  /// Return childrenOrFirstChild without tag
  void* getPtr(void) const;
  /// Return childrenOrFirstChild as integer
  int getFirstChild(void) const;

protected:

  /** The number of children, in case it is greater than 2, or the first
   *  child (if negative)
   */
  int noOfChildren;

  /// Return whether this node is undetermined
  bool isUndetermined(void) const;

  /// Return index of child no \a n
  int getChild(int n) const;
public:
  typedef NodeAllocatorBase<VisualNode> NodeAllocator;

  /// Construct node with parent \a p
  Node(int p, bool failed = false);

  /// Return the parent
  int getParent(void) const;
  /// Return the parent
  VisualNode* getParent(const NodeAllocator& na) const;
  /// Return child no \a n
  VisualNode* getChild(const NodeAllocator& na, int n) const;

  /// Return index of this node
  int getIndex(const NodeAllocator& na) const;

  int getNewIndex(void) const;

  /// Check if this node is the root of a tree
  bool isRoot(void) const;

  /// Set the number of children to \a n and initialize children
  void setNumberOfChildren(unsigned int n, NodeAllocator& na);

  /// Return the number of children
  unsigned int getNumberOfChildren(void) const;

};

#endif // NODE_HH

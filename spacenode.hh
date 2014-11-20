#ifndef SPACENODE_HH
#define SPACENODE_HH

#include "node.hh"

/** \brief Status of nodes in the search tree
 */
enum NodeStatus {
  SOLVED,       ///< Node representing a solution
  FAILED,       ///< Node representing failure
  BRANCH,       ///< Node representing a branch
  UNDETERMINED, ///< Node that has not been explored yet
  STOP,         ///< Node representing stop point
  UNSTOP,       ///< Node representing ignored stop point
  SKIPPED       ///< Skipped by backjumping (basically failed)
};

static const unsigned int FIRSTBIT = 23; //< First free bit in status word
static const unsigned int STATUSMASK = 7<<19; //< Mask for accessing status
static const unsigned int MAXDISTANCE = (1<<19)-1; //< Maximum representable distance
static const unsigned int DISTANCEMASK = (1<<19)-1; //< Mask for accessing distance

class TreeCanvas;
class Data;

/// %Statistics about the search tree
class Statistics {
public:
  /// Number of solutions
  int solutions;
  /// Number of failures
  int failures;
  /// Number of choice nodes
  int choices;
  /// Number of open, undetermined nodes
  int undetermined;
  /// Maximum depth of the tree
  int maxDepth;

  /// Constructor
  Statistics(void)
    : solutions(0), failures(0), choices(0), undetermined(1), maxDepth(0) {}
};

class SpaceNode;

/// \brief A node of a search tree of %Gecode spaces
class SpaceNode : public Node {

    friend class TreeBuilder;
protected:
  /// Reference to node in database
//  int db_id;
protected:

  /** \brief Status of the node
   *
   * If the node has a working copy, the first 20 bits encode the distance
   * to the closest copy. The next 5 bits encode the NodeStatus, and the
   * remaining bits are used by the VisualNode class for further flags.
   */
  unsigned int nstatus;

  /// Set status flag
  void setFlag(int flag, bool value);

  /// Return status flag
  bool getFlag(int flag) const;

  /// Flags for SpaceNodes
  enum SpaceNodeFlags {
    HASOPENCHILDREN = FIRSTBIT,
    HASFAILEDCHILDREN,
    HASSOLVEDCHILDREN
  };
  /// Last bit used for SpaceNode flags
  static const int LASTBIT = HASSOLVEDCHILDREN;

private:
  /// Set whether the node has children that are not fully explored
  void setHasOpenChildren(bool b);
  /// Set whether the subtree of this node is known to contain failure
  void setHasFailedChildren(bool b);
  /// Set whether the subtree of this node is known to contain solutions
  void setHasSolvedChildren(bool b);

  /// Book-keeping of open children
  void closeChild(const NodeAllocator& na,
                  bool hadFailures, bool hadSolutions);
public: // !TODO: change back to protected
  /// Set status to \a s
  void setStatus(NodeStatus s);
public:
  /// Construct node with parent \a p
  SpaceNode(int p);
  /// Construct root node
  SpaceNode(bool);

  /// Free allocated memory
  void dispose(void);

  /** \brief Compute and return the number of children
    *
    * On a node whose status is already determined, this function
    * just returns the number of children.  On an undetermined node,
    * it first acquires a Space (possibly through recomputation), and
    * then asks for its status.  If the space is solved or failed, the
    * node's status will be set accordingly, and 0 will be returned.
    * Otherwise, the status is SS_BRANCH, and as many new children will
    * be created as the branch has alternatives, and the number returned.
    */


  /// Return current status of the node
  NodeStatus getStatus(void) const;

  /// Return whether this node still has open children
  bool isOpen(void);
  /// Return whether the subtree of this node has any failed children
  bool hasFailedChildren(void);
  /// Return whether the subtree of this node has any solved children
  bool hasSolvedChildren(void);
  /// Return whether the subtree of this node has any open children
  bool hasOpenChildren(void);
  /// Return number of open children
  int getNoOfOpenChildren(const NodeAllocator& na);
  /// Set number of open children to \a n
  void setNoOfOpenChildren(int n);

  /// Return alternative number of this node
  int getAlternative(const NodeAllocator& na) const;
};

#endif // SPACENODE_HH

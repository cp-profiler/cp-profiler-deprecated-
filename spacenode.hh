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
  SKIPPED,       ///< Skipped by backjumping (basically failed)
  MERGING
};

static const unsigned int STATUSSTART = 28;

static const unsigned int FIRSTFLAG = 1; //< Number of first flag in status word
static const unsigned int STATUSMASK = 0xF << STATUSSTART; //< Mask for accessing status

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
// class TreeComparison;

/// \brief A node of a search tree of %Gecode spaces
class SpaceNode : public Node {

    friend class TreeBuilder;
    friend class TreeComparison;

protected:
  /// Reference to node in database
//  int db_id;
protected:

  /** \brief Status of the node
   *
   * Anatomy of nstatus
   *
   *                 Subtree
   *                  Size |
   *   Node                |    flags
   *   Status              | /--------\
   *                       | BHOMHCDHHH
   *   /--\               /-\MINRILRSFO
   *   SSSS               SSSKGPKDDTCCC
   *   ********************************
   *    3         2         1         0
   *   10987654321098765432109876543210
   *
   * NOTE THAT THE FLAG NUMBERS DO NOT MATCH THE BIT NUMBERS.
   * (e.g. HASOPENCHILDREN=1 but resides at bit 0)
   * setFlag and getFlag subtract one to correct for this.
   * (setNumericFlag/getNumericFlag do it too.)
   *
   */
  unsigned int nstatus;

  /// Set status flag
  void setFlag(int flag, bool value);

  /// Return status flag
  bool getFlag(int flag) const;

  /// Return nstatus for copying nodes
  // unsigned int getStatusAndFlags(void) const;

  /// Set status numeric flag
  void setNumericFlag(int flag, int size, unsigned int value);

  /// Get status numeric flag
  unsigned int getNumericFlag(int flag, int size) const;

  /// Flags for SpaceNodes
  enum SpaceNodeFlags {
    HASOPENCHILDREN = FIRSTFLAG,
    HASFAILEDCHILDREN,
    HASSOLVEDCHILDREN
  };
  /// Last flag used for SpaceNode flags
  static const int LASTFLAG = HASSOLVEDCHILDREN;

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
protected:
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

#ifndef GECODE_GIST_LAYOUTCURSOR_HH
#define GECODE_GIST_LAYOUTCURSOR_HH

#include "nodecursor.hh"
#include "visualnode.hh"


/// \brief A cursor that computes a tree layout for VisualNodes
class LayoutCursor : public NodeCursor<VisualNode> {
public:
    /// Constructor
    LayoutCursor(VisualNode* theNode,
                 const VisualNode::NodeAllocator& na);

    /// \name Cursor interface
    //@{
    /// Test if the cursor may move to the first child node
    bool mayMoveDownwards(void);
    /// Compute layout for current node
    void processCurrentNode(void);
    //@}
};

#include "layoutcursor.hpp"

#endif

// STATISTICS: gist-any

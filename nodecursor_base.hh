#ifndef NODECURSOR_BASE_HH
#define NODECURSOR_BASE_HH


class NodeAllocator;

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

#endif

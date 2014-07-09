
inline bool
LayoutCursor::mayMoveDownwards(void) {
    return NodeCursor<VisualNode>::mayMoveDownwards() &&
            node()->isDirty();
}

inline
LayoutCursor::LayoutCursor(VisualNode* theNode,
                           const VisualNode::NodeAllocator& na)
    : NodeCursor<VisualNode>(theNode,na) {}

inline void
LayoutCursor::processCurrentNode(void) {
    VisualNode* currentNode = node();
    if (currentNode->isDirty()) {
        if (currentNode->isHidden()) {
            // do nothing
        } else if (false && currentNode->getNumberOfChildren() < 1) {
            currentNode->setShape(Shape::leaf);
        } else {
            currentNode->computeShape(na);
        }
        currentNode->setDirty(false);
    }
    if (currentNode->getNumberOfChildren() >= 1)
        currentNode->setChildrenLayoutDone(true);
}

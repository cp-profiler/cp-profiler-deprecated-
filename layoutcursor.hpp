/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
        } else if (false && currentNode->getNumberOfChildren() < 1) { /// TODO: ask Guido
            currentNode->setShape(Shape::leaf);
        } else {
            currentNode->computeShape(na);
        }
        currentNode->setDirty(false);
    }
    if (currentNode->getNumberOfChildren() >= 1)
        currentNode->setChildrenLayoutDone(true);
}

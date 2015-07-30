/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

inline void
DrawingCursor::moveUpwards(void) {
    x -= node()->getOffset();
    y -= Layout::dist_y;
    NodeCursor<VisualNode>::moveUpwards();
}

inline bool
DrawingCursor::isClipped(void) {
    if (clippingRect.width() == 0 && clippingRect.x() == 0
            && clippingRect.height() == 0 && clippingRect.y() == 0)
        return false;
    BoundingBox b = node()->getBoundingBox();
    return (x + b.left > clippingRect.x() + clippingRect.width() ||
            x + b.right < clippingRect.x() ||
            y > clippingRect.y() + clippingRect.height() ||
            y + (node()->getShape()->depth()+1) * Layout::dist_y <
            clippingRect.y());
}

inline bool
DrawingCursor::mayMoveDownwards(void) {
    return NodeCursor<VisualNode>::mayMoveDownwards() &&
            !node()->isHidden() &&
            node()->childrenLayoutIsDone() &&
            !isClipped();
}

inline void
DrawingCursor::moveDownwards(void) {
    NodeCursor<VisualNode>::moveDownwards();
    x += node()->getOffset();
    y += Layout::dist_y;
}

inline void
DrawingCursor::moveSidewards(void) {
    x -= node()->getOffset();
    NodeCursor<VisualNode>::moveSidewards();
    x += node()->getOffset();
}

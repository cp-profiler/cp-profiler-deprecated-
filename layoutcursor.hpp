/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Guido Tack, 2006
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

inline bool
LayoutCursor::mayMoveDownwards(void) {
    return NodeCursor::mayMoveDownwards() &&
            node()->isDirty();
}

inline
LayoutCursor::LayoutCursor(VisualNode* theNode,
                           const NodeAllocator& na)
    : NodeCursor(theNode,na) {}


static Shape* sizedRectangleLevels(int levels) {

    using sized_rect::HALF_WIDTH;

    // if (levels == 1) levels = 2;

    auto shape = Shape::allocate(levels);
    (*shape)[0] = Extent(-HALF_WIDTH, HALF_WIDTH);

    if (levels > 1) {
        (*shape)[1] = Extent(0, 0);
    }

    for (int l = 2; l < levels; ++l) {
        (*shape)[l] = Extent(0, 0);
    }

    shape->computeBoundingBox();

    return shape;
}

/// size is between 0 and 127
static Shape* sizedRectangle(int size) {

    using sized_rect::K; using sized_rect::BASE_HEIGHT;

    /// Figure out how many layers needed:
    int n = std::ceil((size * K + BASE_HEIGHT) / Layout::dist_y) + 1;

    auto shape = sizedRectangleLevels(n);
    return shape;
}

inline void
LayoutCursor::processCurrentNode(void) {
    VisualNode* currentNode = node();
    // qDebug() << "LayoutCursor visiting node " << currentNode->debug_id << " whose dirtiness is" << currentNode->isDirty();
    if (currentNode->isDirty()) {
        // std::cerr << "LayoutCurser: node is dirty\n";
        if (currentNode->isHidden()) {
            // do nothing
            auto shape = sizedRectangle(currentNode->getSubtreeSize());
            currentNode->setShape(shape);
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

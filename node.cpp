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
#include "node.hh"
#include "visualnode.hh"
#include <cassert>

#ifdef MAXIM_DEBUG
 int Node::debug_instance_counter = -1;
#endif

void
Node::setNumberOfChildren(unsigned int n, NodeAllocator& na) {
    assert(getTag() == UNDET);
    switch (n) {
    case 0:
        setTag(LEAF);
        break;
    case 1:
        childrenOrFirstChild =
                reinterpret_cast<void*>(na.allocate(getIndex(na)) << 2);
//                reinterpret_cast<void*>(na.allocate(getNewIndex()) << 2);
        noOfChildren = 1;
        setTag(TWO_CHILDREN);
        break;
    case 2:
    {
        childrenOrFirstChild =
                reinterpret_cast<void*>(na.allocate(getIndex(na)) << 2);
//                reinterpret_cast<void*>(na.allocate(getNewIndex()) << 2);

        noOfChildren = -na.allocate(getIndex(na));
//        noOfChildren = -na.allocate(getNewIndex());
        setTag(TWO_CHILDREN);
    }
        break;
    default:
    {
        noOfChildren = n;
        int* children = heap.alloc<int>(n);
        childrenOrFirstChild = static_cast<void*>(children);
        setTag(MORE_CHILDREN);
        for (unsigned int i=n; i--;)
            children[i] = na.allocate(getIndex(na));
//            children[i] = na.allocate(getNewIndex());
    }
    }
}

#ifdef MAXIM_DEBUG
void
Node::removeChild(int n, NodeAllocator& na) {
    /// ***** one child *****


    /// ***** two children *****

        /// 1. make number of node -> 1
        /// 2. if first child -> place second instead of the first

        // qDebug() << "tag: " << getTag();
    switch(getTag()) {
        case TWO_CHILDREN:

            if (n == getFirstChild()) {
                setFirstChild(-noOfChildren);
            }

            if (noOfChildren <= 0) {
                /// actually had two children -> now one
                noOfChildren = 1;
            } else {
                /// only had one child -> now a leaf
                resetTag(LEAF);
            }

        break;

        case MORE_CHILDREN:
#ifdef MAXIM_DEBUG
            qDebug() << "(!) feature not implemented: remove a child if the parent has > 2 nodes";
#endif
        break;

        default:
            assert(false);
    }

    /// ***** more children *****
}
#endif

int
Node::addChild(NodeAllocator &na) {
    switch (getNumberOfChildren()) {
    case 0:
        childrenOrFirstChild =
                reinterpret_cast<void*>(na.allocate(getIndex(na)) << 2);
        noOfChildren = 1;
        setTag(TWO_CHILDREN);
        assert(getNumberOfChildren()==1);
        return getFirstChild();
    case 1:
        noOfChildren = -na.allocate(getIndex(na));
        assert(getNumberOfChildren()==2);
        return -noOfChildren;
    case 2:
    {
        int* children = heap.alloc<int>(3);
        children[0] = getFirstChild();
        children[1] = -noOfChildren;
        children[2] = na.allocate(getIndex(na));
        noOfChildren = 3;
        childrenOrFirstChild = static_cast<void*>(children);
        setTag(MORE_CHILDREN);
        assert(getNumberOfChildren()==3);
        return children[2];
    }
    default:
    {
        noOfChildren++;
        int* oldchildren = static_cast<int*>(getPtr());
        int* newchildren = heap.realloc<int>(oldchildren,noOfChildren-1,noOfChildren);
        // for (int i=0; i<noOfChildren-1; i++)
        //     assert(oldchildren[i]==newchildren[i]);
        newchildren[noOfChildren-1] = na.allocate(getIndex(na));
        childrenOrFirstChild = static_cast<void*>(newchildren);
        setTag(MORE_CHILDREN);
        assert(static_cast<int>(getNumberOfChildren())==noOfChildren);
        return newchildren[noOfChildren-1];
    }
    }
}
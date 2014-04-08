#include "node.hh"
#include "visualnode.hh"
#include <cassert>

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
        // idx = getIndex(na);
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

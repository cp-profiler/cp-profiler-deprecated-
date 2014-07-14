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
        assert(getNumberOfChildren()==noOfChildren);
        return newchildren[noOfChildren-1];
    }
    }
}

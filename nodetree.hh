#ifndef NODETREE_HH
#define NODETREE_HH

#include "visualnode.hh"

class NodeTree {
private:
    NodeAllocator na;
    VisualNode* root;
    Statistics stats;

public:
    NodeTree() {
        int rootIdx = na.allocateRoot();
        assert(rootIdx == 0);
        (void)rootIdx;
        root = na[0];
        root->setMarked(true);
    }
    const NodeAllocator& getNA() const {
        return na;
    }
    NodeAllocator& getNA() {
        return na;
    }

    VisualNode* getRootNode() const {
        return root;
    }
};

#endif

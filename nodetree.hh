#ifndef NODETREE_HH
#define NODETREE_HH

#include <QMutex>
#include "visualnode.hh"

class NodeTree {
private:
    /// Mutex for synchronizing acccess to the tree
    QMutex mutex;
    /// Mutex for synchronizing layout and drawing
    QMutex layoutMutex;
    NodeAllocator na;
    Statistics stats;
public:
    NodeTree()
        : mutex(QMutex::Recursive),
          layoutMutex(QMutex::Recursive) {
        int rootIdx = na.allocateRoot();
        assert(rootIdx == 0);
        (void)rootIdx;
        na[0]->setMarked(true);
    }
    const NodeAllocator& getNA() const {
        return na;
    }
    NodeAllocator& getNA() {
        return na;
    }

    VisualNode* getRootNode() const {
        return na[0];
    }

    VisualNode* getChild(const VisualNode& node, int alt) const {
        return na[node.getChild(alt)];
    }

    Statistics& getStatistics() {
        return stats;
    }

    int calculateDepth(const VisualNode& node) const {
        int count = 0;

        auto it = &node;

        while (it = it->getParent(na)) { count++; }

        return count;
    }

    QMutex& getMutex() { return mutex; }
    QMutex& getLayoutMutex() { return layoutMutex; }
};

#endif

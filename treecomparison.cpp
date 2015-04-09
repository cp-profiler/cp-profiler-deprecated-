#include "treecomparison.hh"

TreeComparison::TreeComparison(void)
: no_pentagons(0) {}

void
TreeComparison::compare(TreeCanvas* t1, TreeCanvas* t2, TreeCanvas* new_tc) {
    Node::NodeAllocator* na1 = t1->na;
    Node::NodeAllocator* na2 = t2->na;
    VisualNode* root1 = (*na1)[0];
    VisualNode* root2 = (*na2)[0];

    VisualNode* next;

    stack1.push(root1);
    stack2.push(root2);

    bool rootBuilt = false;

    Node::NodeAllocator* na = new_tc->na;

    TreeComparison::setSource(na1, na2, t1->_data, t2->_data);

    while (stack1.size() > 0) {
        VisualNode* node1 = stack1.pop();
        VisualNode* node2 = stack2.pop();
        bool equal = TreeComparison::copmareNodes(node1, node2);
        if (equal) {
            uint kids = node1->getNumberOfChildren();
            for (uint i = 0; i < kids; ++i) {
                stack1.push(node1->getChild(*na1, i));
                stack2.push(node2->getChild(*na2, i));
            }

            /// if roots are equal
            if (!rootBuilt) {
                next = new_tc->root;
                rootBuilt = true;
            } else {
                next = stack.pop();
            }

            /// new node is built
            
            next->setNumberOfChildren(kids, *na);
            next->setStatus(node1->getStatus());
            next->_tid = 0;

            /// point to the source node

            unsigned int source_index = node2->getIndex(*na2);
            unsigned int target_index = next->getIndex(*na);

            DbEntry* entry = t2->_data->getEntry(source_index);
            new_tc->_data->connectNodeToEntry(target_index, entry);

            for (unsigned int i = 0; i < kids; ++i) {
                stack.push(next->getChild(*na, i));
            }

        } else {
            /// not equal

            no_pentagons++;

            next = stack.pop();
            next->setNumberOfChildren(2, *na);
            next->setStatus(MERGING);
            next->setHidden(true);
            next->_tid = 0;

            stack.push(next->getChild(*na, 1));
            stack.push(next->getChild(*na, 0));

            copyTree(stack.pop(), new_tc, node1, t1, 1);
            copyTree(stack.pop(), new_tc, node2, t2, 2);

        }

        next->dirtyUp(*na);
        new_tc->update();
    }
}

void 
TreeComparison::copyTree(VisualNode* target, TreeCanvas* tc,
                         VisualNode* root,   TreeCanvas* tc_source, int which) {

    NodeAllocator* na = tc->na;
    NodeAllocator* na_source = tc_source->na; 
    
    QStack<VisualNode*>* source_stack = new QStack<VisualNode*>();
    QStack<VisualNode*>* target_stack = new QStack<VisualNode*>();

    source_stack->push(root);
    target_stack->push(target);

    while (source_stack->size() > 0) {
        VisualNode* n = source_stack->pop();
        VisualNode* next = target_stack->pop();

        next->_tid = which; // treated as a colour

        uint kids = n->getNumberOfChildren();
        next->setNumberOfChildren(kids, *na);
        next->setStatus(n->getStatus());

        /// point to the source node

        unsigned int source_index = n->getIndex(*na_source);
        unsigned int target_index = next->getIndex(*na);

        DbEntry* entry = tc_source->_data->getEntry(source_index);
        tc->_data->connectNodeToEntry(target_index, entry);

        next->dirtyUp(*na);

        for (uint i = 0; i < kids; ++i) {
            source_stack->push(n->getChild(*na_source, i));
            target_stack->push(next->getChild(*na, i));
        }
    }

    delete source_stack;
    delete target_stack;
}

bool
TreeComparison::copmareNodes(VisualNode* n1, VisualNode* n2) {
    if (n1->getNumberOfChildren() != n2->getNumberOfChildren())
        return false;

    if (n1->getStatus() != n2->getStatus()) 
        return false;

    int id1 = n1->getIndex(*_na1);
    int id2 = n2->getIndex(*_na2);

    if (
        strcmp(
            _data1->getLabelByGid(id1),
            _data2->getLabelByGid(id2)
        ) != 0
    ) { 
        // qDebug() << "failed on labels comparison";
        return false;
    }

    return true;
}

void
TreeComparison::setSource(NodeAllocator* na1, NodeAllocator* na2,
                          Data* data1, Data* data2) {
    _na1 = na1;
    _na2 = na2;
    _data1 = data1;
    _data2 = data2;
}

int
TreeComparison::get_no_pentagons(void) {
    return no_pentagons;
}
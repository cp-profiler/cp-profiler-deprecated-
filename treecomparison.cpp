#include "treecomparison.hh"
#include "treecanvas.hh"
#include "node.hh"

TreeComparison::TreeComparison(void) {}

void
TreeComparison::compare(TreeCanvas* t1, TreeCanvas* t2, TreeCanvas* new_tc) {
    Node::NodeAllocator* na1 = t1->na;
    Node::NodeAllocator* na2 = t2->na;
    VisualNode* root1 = (*na1)[0];
    VisualNode* root2 = (*na2)[0];

    VisualNode* next;

    stack1.push(root1);
    stack2.push(root2);

    VisualNode* root = (*new_tc->na)[0];
    stack.push(root);

    bool rootBuilt = false;

    Node::NodeAllocator* na = new_tc->na;

    TreeComparison::setSource(na1, na2, t1->_data, t2->_data);

    while (stack1.size() > 0) {
        VisualNode* node1 = stack1.pop();
        VisualNode* node2 = stack2.pop();


        /// ---------- Skipping implied ---------------

        int implied_child;

        /// check if any children of node 1 are implied
        /// if so, skip this node (stack1.pop())

        do {
            implied_child = -1;

            unsigned int kids = node1->getNumberOfChildren();
            for (unsigned int i = 0; i < kids; i++) {

                int child_gid = node1->getChild(i);
                const char* label = _data1->getLabelByGid(child_gid);

                /// check if label starts with "[i]"

                if (strncmp(label, "[i]", 3) == 0){
                    implied_child = i;
                    break;
                    qDebug() << "found implied: " << label;
                }
            }

            /// if implied not found -> continue,
            /// otherwise skip this node
            if (implied_child != -1) {
                node1 = node1->getChild(*na1, implied_child);
            }
        } while (implied_child != -1);


        /// the same for node 2

        do {
            implied_child = -1;

            unsigned int kids = node2->getNumberOfChildren();
            for (unsigned int i = 0; i < kids; i++) {

                int child_gid = node2->getChild(i);
                const char* label = _data2->getLabelByGid(child_gid);

                /// check if label starts with "[i]"

                if (strncmp(label, "[i]", 3) == 0){
                    implied_child = i;
                    break;
                    qDebug() << "found implied: " << label;
                }
            }

            /// if implied not found -> continue,
            /// otherwise skip this node
            if (implied_child != -1) {
                node2 = node2->getChild(*na2, implied_child);
            }
        } while (implied_child != -1);


        /// ----------------------------------------------------

        bool equal = TreeComparison::copmareNodes(node1, node2);
        if (equal) {
            uint kids = node1->getNumberOfChildren();
            for (uint i = 0; i < kids; ++i) {
                stack1.push(node1->getChild(*na1, kids - i - 1));
                stack2.push(node2->getChild(*na2, kids - i - 1));
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
            // next->setStatus(node1->getStatus());
            next->nstatus = node1->nstatus;
            next->_tid = 0;

            /// point to the source node

            unsigned int source_index = node2->getIndex(*na2);
            unsigned int target_index = next->getIndex(*na);

            DbEntry* entry = t2->_data->getEntry(source_index);
            new_tc->_data->connectNodeToEntry(target_index, entry);

            for (unsigned int i = 0; i < kids; ++i) {
                stack.push(next->getChild(*na, kids - i - 1));
            }

        } else {
            /// not equal

            next = stack.pop();
            next->setNumberOfChildren(2, *na);
            next->setStatus(MERGING);
            next->getParent(*na)->setHidden(false);
            next->setHidden(true);
            next->_tid = 0;

            _pentagons.push_back(next);

            stack.push(next->getChild(*na, 1));
            stack.push(next->getChild(*na, 0));

            unsigned int left = copyTree(stack.pop(), new_tc, node1, t1, 1);
            unsigned int right = copyTree(stack.pop(), new_tc, node2, t2, 2);

            _pentSize.push_back(std::make_pair(left, right));

        }

        next->dirtyUp(*na);
        new_tc->update();
    }
}

unsigned int
TreeComparison::copyTree(VisualNode* target, TreeCanvas* tc,
                         VisualNode* root,   TreeCanvas* tc_source, int which) {

    NodeAllocator* na = tc->na;
    NodeAllocator* na_source = tc_source->na; 
    
    QStack<VisualNode*>* source_stack = new QStack<VisualNode*>();
    QStack<VisualNode*>* target_stack = new QStack<VisualNode*>();

    source_stack->push(root);
    target_stack->push(target);

    unsigned int count = 0;

    while (source_stack->size() > 0) {
        count++;

        VisualNode* n = source_stack->pop();
        VisualNode* next = target_stack->pop();

        next->_tid = which; // treated as a colour

        uint kids = n->getNumberOfChildren();
        next->setNumberOfChildren(kids, *na);
        // next->setStatus(n->getStatus());
        next->nstatus = n->nstatus;

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

    return count;
}

bool
TreeComparison::copmareNodes(VisualNode* n1, VisualNode* n2) {
    unsigned int kids = n1->getNumberOfChildren();
    if (kids != n2->getNumberOfChildren())
        return false;

    if (n1->getStatus() != n2->getStatus()) 
        return false;

    // int id1 = n1->getIndex(*_na1);
    // int id2 = n2->getIndex(*_na2);

    for (auto i = 0; i < kids; i++) {

        int id1 = n1->getChild(i);
        int id2 = n2->getChild(i);

        if (
            strcmp(
                _data1->getLabelByGid(id1),
                _data2->getLabelByGid(id2)
            ) != 0
        ) {
            return false;
        }
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
    return static_cast<int>(_pentagons.size());
}
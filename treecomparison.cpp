#include "treecomparison.hh"

QStack<VisualNode*> TreeComparison::stack;
QStack<VisualNode*> TreeComparison::stack1;
QStack<VisualNode*> TreeComparison::stack2;

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

    while (stack1.size() > 0) {
        VisualNode* node1 = stack1.pop();
        VisualNode* node2 = stack2.pop();
        bool equal = TreeComparison::copmareNodes(node1, node2);
        if (equal) {
            uint kids = node1->getNumberOfChildren();
            for (int i = 0; i < kids; ++i) {
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

            next->setNumberOfChildren(kids, *na);
            next->setStatus(node1->getStatus());
            next->_tid = 0;

            for (int i = 0; i < kids; ++i) {
                stack.push(next->getChild(*na, i));
            }

        } else {
            /// not equal

            next = stack.pop();
            next->setNumberOfChildren(2, *na);
            next->setStatus(UNDETERMINED);
            next->_tid = 0;

            stack.push(next->getChild(*na, 1));
            stack.push(next->getChild(*na, 0));

            copyTree(stack.pop(), na, node1, na1, 1);
            copyTree(stack.pop(), na, node2, na2, 2);

        }

        next->dirtyUp(*na);
        new_tc->update();
    }
}

void 
TreeComparison::copyTree(VisualNode* target, NodeAllocator* na,
                         VisualNode* root,   NodeAllocator* na_source, int which) {
    
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

        next->dirtyUp(*na);

        for (int i = 0; i < kids; ++i) {
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

    /// TODO: add labels as well

    if (n1->getStatus() != n2->getStatus()) 
        return false;
    return true;
}

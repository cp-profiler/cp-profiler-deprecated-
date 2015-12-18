/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "treecomparison.hh"
#include "treecanvas.hh"
#include "node.hh"

TreeComparison::TreeComparison(void) {}

void
TreeComparison::compare(TreeCanvas* t1, TreeCanvas* t2, TreeCanvas* new_tc) {

    /// For source trees
    QStack<VisualNode*> stack1;
    QStack<VisualNode*> stack2;
    /// The stack used while building new_tc
    QStack<VisualNode*> stack;

    Node::NodeAllocator* na1 = t1->na;
    Node::NodeAllocator* na2 = t2->na;
    VisualNode* root1 = (*na1)[0];
    VisualNode* root2 = (*na2)[0];
    Execution* execution1 = t1->execution;
    Execution* execution2 = t2->execution;

    VisualNode* next;

    stack1.push(root1);
    stack2.push(root2);

    VisualNode* root = (*new_tc->na)[0];
    stack.push(root);

    bool rootBuilt = false;

    Node::NodeAllocator* na = new_tc->na;

    TreeComparison::setSource(na1, na2, execution1, execution2);

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
                std::string label = _ex1->getLabel(child_gid);

                /// check if label starts with "[i]"

                if (label.compare(0, 3, "[i]") == 0) {
                    implied_child = i;
                    break;
                    qDebug() << "found implied: " << label.c_str();
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
                std::string label = _ex2->getLabel(child_gid);

                /// check if label starts with "[i]"

                if (label.compare(0, 3, "[i]") == 0){
                    implied_child = i;
                    break;
                    qDebug() << "found implied: " << label.c_str();
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

            DbEntry* entry = _ex2->getEntry(source_index);
            new_tc->getExecution()->getData()->connectNodeToEntry(target_index, entry);

            for (unsigned int i = 0; i < kids; ++i) {
                stack.push(next->getChild(*na, kids - i - 1));
            }

        } else {
            /// not equal

            next = stack.pop();
            next->setNumberOfChildren(2, *na);
            next->setStatus(MERGING);
            if (!next->isRoot())
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

    QStack<VisualNode*> source_stack;
    QStack<VisualNode*> target_stack;

    source_stack.push(root);
    target_stack.push(target);

    unsigned int count = 0;

    while (source_stack.size() > 0) {
        count++;

        VisualNode* n = source_stack.pop();
        VisualNode* next = target_stack.pop();

        next->_tid = which; // treated as a colour

        uint kids = n->getNumberOfChildren();
        next->setNumberOfChildren(kids, *na);
        // next->setStatus(n->getStatus());
        next->nstatus = n->nstatus;

        /// point to the source node
        unsigned int source_index = n->getIndex(*na_source);
        unsigned int target_index = next->getIndex(*na);

        if (n->getStatus() != NodeStatus::UNDETERMINED) {
            DbEntry* entry = tc_source->getExecution()->getData()->getEntry(source_index);
            tc->getExecution()->getData()->connectNodeToEntry(target_index, entry);
        }

        next->dirtyUp(*na);

        for (uint i = 0; i < kids; ++i) {
            source_stack.push(n->getChild(*na_source, i));
            target_stack.push(next->getChild(*na, i));
        }
    }

    return count;
}

bool
TreeComparison::copmareNodes(VisualNode* n1, VisualNode* n2) {
    unsigned kids = n1->getNumberOfChildren();
    if (kids != n2->getNumberOfChildren())
        return false;

    if (n1->getStatus() != n2->getStatus()) 
        return false;

    // int id1 = n1->getIndex(*_na1);
    // int id2 = n2->getIndex(*_na2);

    for (unsigned i = 0; i < kids; i++) {

        int id1 = n1->getChild(i);
        int id2 = n2->getChild(i);

        if (_ex1->getLabel(id1).compare(_ex2->getLabel(id2)) != 0) {
            return false;
        }

    }

    return true;
}

void
TreeComparison::setSource(NodeAllocator* na1, NodeAllocator* na2,
                          Execution* ex1, Execution* ex2) {
    _na1 = na1;
    _na2 = na2;
    _ex1 = ex1;
    _ex2 = ex2;
}

int
TreeComparison::get_no_pentagons(void) {
    return static_cast<int>(_pentagons.size());
}

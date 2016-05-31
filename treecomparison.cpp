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

TreeComparison::TreeComparison(Execution& ex1, Execution& ex2)
    : _ex1(ex1), _ex2(ex2), _na1(ex1.getNA()), _na2(ex2.getNA()) {}

void
TreeComparison::sortPentagons() {
    std::sort(m_pentagonItems.begin(), m_pentagonItems.end(),
        [](const PentagonItem& lhs, const PentagonItem& rhs){
            return std::abs((int)lhs.l_size - (int)lhs.r_size)
                 > std::abs((int)rhs.l_size - (int)rhs.r_size);
    });
}

/// note(maxim): Defined in cmp_tree_dialog.cpp
std::vector<int>
infoToNogoodVector(const string& info);

void
TreeComparison::analyseNogoods(const string& info, int search_reduction) {
    auto ng_vec = infoToNogoodVector(info);

    for (auto ng_id : ng_vec) {
        auto& ng_stats = m_responsibleNogoodStats[ng_id];
        ng_stats.occurrence++;
        ng_stats.search_eliminated += search_reduction / ng_vec.size();
    }

    m_totalReduced += search_reduction;

}

void
TreeComparison::compare(TreeCanvas* new_tc, bool with_labels) {

    /// For source trees
    QStack<const VisualNode*> stack1;
    QStack<const VisualNode*> stack2;
    /// The stack used while building new_tc
    QStack<VisualNode*> stack;

    VisualNode* root1 = _na1[0];
    VisualNode* root2 = _na2[0];

    VisualNode* next;

    stack1.push(root1);
    stack2.push(root2);

    VisualNode* root = execution.getNA()[0];
    stack.push(root);

    bool rootBuilt = false;

    /// TODO(maxim): reset responsible nogood counts?

    NodeAllocator& na = execution.getNA();

    while (stack1.size() > 0) {
        auto node1 = stack1.pop();
        auto node2 = stack2.pop();


        /// ---------- Skipping implied ---------------

        int implied_child;

        /// check if any children of node 1 are implied
        /// if so, skip this node (stack1.pop())

        do {
            implied_child = -1;

            unsigned int kids = node1->getNumberOfChildren();
            for (unsigned int i = 0; i < kids; i++) {

                int child_gid = node1->getChild(i);
                std::string label = _ex1.getLabel(child_gid);

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
                node1 = node1->getChild(_na1, implied_child);
            }
        } while (implied_child != -1);


        /// the same for node 2

        do {
            implied_child = -1;

            unsigned int kids = node2->getNumberOfChildren();
            for (unsigned int i = 0; i < kids; i++) {

                int child_gid = node2->getChild(i);
                std::string label = _ex2.getLabel(child_gid);

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
                node2 = node2->getChild(_na2, implied_child);
            }
        } while (implied_child != -1);


        /// ----------------------------------------------------

        bool equal = TreeComparison::copmareNodes(node1, node2, with_labels);
        if (equal) {
            uint kids = node1->getNumberOfChildren();
            for (uint i = 0; i < kids; ++i) {
                stack1.push(node1->getChild(_na1, kids - i - 1));
                stack2.push(node2->getChild(_na2, kids - i - 1));
            }

            /// if roots are equal
            if (!rootBuilt) {
                next = new_tc->root;
                rootBuilt = true;
            } else {
                next = stack.pop();
            }

            /// new node is built

            next->setNumberOfChildren(kids, na);
            // next->setStatus(node1->getStatus());
            next->nstatus = node1->nstatus;
            next->_tid = 0;

            /// point to the source node

            unsigned int source_index = node2->getIndex(_na2);
            unsigned int target_index = next->getIndex(na);

            DbEntry* entry = _ex2.getEntry(source_index);
            new_tc->getExecution()->getData()->connectNodeToEntry(target_index, entry);

            for (unsigned int i = 0; i < kids; ++i) {
                stack.push(next->getChild(na, kids - i - 1));
            }

        } else {
            /// not equal

            next = stack.pop();
            next->setNumberOfChildren(2, na);
            next->setStatus(MERGING);
            if (!next->isRoot())
                next->getParent(na)->setHidden(false);
            next->setHidden(true);
            next->_tid = 0;

            new_tc->unhideNode(next); /// unhide pentagons if hidden

            stack.push(next->getChild(na, 1));
            stack.push(next->getChild(na, 0));

            int left = copyTree(stack.pop(), new_tc, node1, _ex1, 1);
            int right = copyTree(stack.pop(), new_tc, node2, _ex2, 2);

            /// hide the nodes one level below a pentagon
            /// if they are hidden on the original tree
            assert(next->getNumberOfChildren() == 2);

            if (node1->getNumberOfChildren() > 0 && !node1->isNodeVisible(_ex1.getNA())) {
                next->getChild(na, 0)->setHidden(true);
            }

            if (node2->getNumberOfChildren() > 2 && !node2->isNodeVisible(_ex2.getNA())) {
                next->getChild(na, 1)->setHidden(true);
            }

            const string* info_str = nullptr;
            /// if node1 is FAILED -> check nogoods // TODO(maxim): branch node?
            if (node1->getStatus() == FAILED) {
                info_str = _ex1.getInfo(*node1);

                int search_reduction = -1;
                assert(left == 1);
                search_reduction = right - left;
                /// identify nogoods and increment counters
                if (info_str) analyseNogoods(*info_str, search_reduction);
            }

            m_pentagonItems.emplace_back(PentagonItem{left, right, next, info_str});


        }

        next->dirtyUp(na);
        new_tc->update();
    }
}

int
TreeComparison::copyTree(VisualNode* target, TreeCanvas* tc,
                         const VisualNode* root, const Execution& ex_source, int which) {

    NodeAllocator& na = execution.getNA();
    const NodeAllocator& na_source = ex_source.getNA();

    QStack<const VisualNode*> source_stack;
    QStack<VisualNode*> target_stack;

    source_stack.push(root);
    target_stack.push(target);

    int count = 0;

    while (source_stack.size() > 0) {
        count++;

        const VisualNode* n = source_stack.pop();
        VisualNode* next = target_stack.pop();

        next->_tid = which; // treated as a colour

        uint kids = n->getNumberOfChildren();
        next->setNumberOfChildren(kids, na);
        // next->setStatus(n->getStatus());
        next->nstatus = n->nstatus;

        /// point to the source node
        unsigned int source_index = n->getIndex(na_source);
        unsigned int target_index = next->getIndex(na);

        if (n->getStatus() != NodeStatus::UNDETERMINED) {
            auto source_data = ex_source.getData();
            DbEntry* entry = source_data->getEntry(source_index);

            auto this_data = tc->getExecution()->getData();
            this_data->connectNodeToEntry(target_index, entry);

            /// TODO(maxim): connect nogoods as well

            auto sid = entry->s_node_id;
            auto info = source_data->sid2info.find(sid);

            /// note(maxim): should have to maintain another map
            /// (even though info is only a pointer)
            if (info != source_data->sid2info.end()) {
                this_data->sid2info[sid] = info->second;
            }

        }

        next->dirtyUp(na);

        for (uint i = 0; i < kids; ++i) {
            source_stack.push(n->getChild(na_source, i));
            target_stack.push(next->getChild(na, i));
        }
    }

    return count;
}

void find_and_replace_all(std::string& str, std::string substr_old, std::string substr_new) {
    auto pos = str.find(substr_old);
    while (pos != std::string::npos) {
        str.replace(pos, std::string(substr_old).length(), substr_new);
        pos = str.find(substr_old);
    }
}

bool
TreeComparison::copmareNodes(const VisualNode* n1, const VisualNode* n2, bool with_labels) {
    unsigned kids = n1->getNumberOfChildren();
    if (kids != n2->getNumberOfChildren())
        return false;

    if (n1->getStatus() != n2->getStatus())
        return false;

    /// check labels
    if (with_labels) {
        for (unsigned i = 0; i < kids; i++) {

            int id1 = n1->getChild(i);
            int id2 = n2->getChild(i);

            auto label1 = _ex1.getLabel(id1);
            auto label2 = _ex2.getLabel(id2);

            /// NOTE(maxim): removes whitespaces before comparing;
            /// this will be necessary as long as Chuffed and Gecode don't agree
            /// on whether to put whitespaces around operators (Gecode uses ' '
            /// for parsing logbrancher while Chuffed uses them as a delimiter
            /// between literals)

            label1.erase(remove_if(label1.begin(), label1.end(), isspace), label1.end());
            label2.erase(remove_if(label2.begin(), label2.end(), isspace), label2.end());

            /// Replace `==` with `=`

            find_and_replace_all(label1, "==", "=");
            find_and_replace_all(label2, "==", "=");

            if (label1.compare(label2) != 0) {
                qDebug() << "labels not equal: " << label1.c_str() << " " << label2.c_str();
                return false;
            }

        }
    }

    return true;
}

int
TreeComparison::get_no_pentagons(void) {
    return static_cast<int>(m_pentagonItems.size());
}

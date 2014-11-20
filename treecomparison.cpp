#include "treecomparison.hh"

QStack<VisualNode*> TreeComparison::stack1;
QStack<VisualNode*> TreeComparison::stack2;

void
TreeComparison::compare(TreeCanvas* t1, TreeCanvas* t2, TreeCanvas* new_tc) {
    Node::NodeAllocator* na1 = t1->na;
    Node::NodeAllocator* na2 = t2->na;
    VisualNode* root1 = (*na1)[0];
    VisualNode* root2 = (*na2)[0];
    stack1.push(root1);
    stack2.push(root2);

    while (stack1.size() > 0) {
        VisualNode* node1 = stack1.pop();
        VisualNode* node2 = stack2.pop();
        bool equal = TreeComparison::copmareNodes(node1, node2);
        if (equal) {
            for (int i = 0; i < node1->getNumberOfChildren(); ++i) {
                stack1.push(node1->getChild(*na1, i));
                stack2.push(node2->getChild(*na2, i));
            }
            qDebug() << "equal";
        }
    }
}

bool
TreeComparison::copmareNodes(VisualNode* n1, VisualNode* n2) {
    if (n1->getNumberOfChildren() != n2->getNumberOfChildren())
        return false;

    // qDebug() << "status:" << n1->getStatus() << "vs" << n2->getStatus();

    if (n1->getStatus() != n2->getStatus()) 
        return false;
    return true;
}

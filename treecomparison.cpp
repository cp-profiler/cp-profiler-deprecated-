#include "treecomparison.hh"

void
TreeComparison::compare(TreeCanvas* t1, TreeCanvas* t2) {
    Node::NodeAllocator* na1 = t1->na;
    Node::NodeAllocator* na2 = t2->na;
    VisualNode* root1 = (*na1)[0];
    VisualNode* root2 = (*na2)[0];

    qDebug() << TreeComparison::copmareNodes(root1, root2);
    

}

bool
TreeComparison::copmareNodes(VisualNode* n1, VisualNode* n2) {
    if (n1->getNumberOfChildren() != n2->getNumberOfChildren())
        return false;
    if (n1->getStatus() != n2->getStatus())
        return false;
    return true;
}

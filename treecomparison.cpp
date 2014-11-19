#include "treecomparison.hh"

void
TreeComparison::compare(TreeCanvas* t1, TreeCanvas* t2) {
    Node::NodeAllocator* na1 = t1->na;
    VisualNode* root = (*na1)[0];
    qDebug() << "kids root: " << root->getNumberOfChildren();
    VisualNode* kid1 = root->getChild(*na1, 0);
    VisualNode* kid2 = root->getChild(*na1, 1);
    qDebug() << "kids node1: " << kid1->getNumberOfChildren();
    qDebug() << "kids node2: " << kid2->getNumberOfChildren();
    qDebug() << "status1: " << kid1->getStatus();
    qDebug() << "status2: " << kid2->getStatus();


    // Node::NodeAllocator* na2 = t2->na;
}

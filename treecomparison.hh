#ifndef TREE_COMPARISON
#define TREE_COMPARISON

#include "treecanvas.hh"

class TreeComparison {
public:
  static void compare(TreeCanvas* t1, TreeCanvas* t2);

private:
  /// Return true/false depending on whether n1 ~ n2
  static bool copmareNodes(VisualNode* n1, VisualNode* n2); /// TODO: make it inline?

};

#endif

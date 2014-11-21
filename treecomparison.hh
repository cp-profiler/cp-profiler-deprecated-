#ifndef TREE_COMPARISON
#define TREE_COMPARISON


#include <QStack>
#include "treecanvas.hh"

// Two stacks or a stack of pairs?

class TreeComparison {
public:
  static void compare(TreeCanvas* t1, TreeCanvas* t2, TreeCanvas* new_tc);

private:
  static QStack<VisualNode*> stack1;
  static QStack<VisualNode*> stack2;

  /// The stack used while building new_tc
  static QStack<VisualNode*> stack;
  /// Return true/false depending on whether n1 ~ n2
  static bool copmareNodes(VisualNode* n1, VisualNode* n2); /// TODO: make it inline?

  /// 'which' is treated as a colour, usually 1 or 2 depending on which tree is a source
  static void copyTree(VisualNode*, NodeAllocator*, 
                       VisualNode*, NodeAllocator*, int which = 0);

};

#endif

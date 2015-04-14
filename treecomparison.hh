#ifndef TREE_COMPARISON
#define TREE_COMPARISON

#include "treecanvas.hh"

#include <QStack>
#include <vector>
#include <utility>


// Two stacks or a stack of pairs?

class TreeCanvas;
class Data;
class VisualNode;
class Node;

class TreeComparison {


public:
  TreeComparison(void);
  void compare(TreeCanvas* t1, TreeCanvas* t2, TreeCanvas* new_tc);

  int get_no_pentagons(void);

  inline const std::vector<VisualNode*>& pentagons(void) { return _pentagons; }
  inline const std::vector<std::pair<unsigned int, unsigned int>>& pentSize(void) { return _pentSize;}

private:
  std::vector<VisualNode*> _pentagons;
  std::vector<std::pair<unsigned int, unsigned int>> _pentSize;

private:
  QStack<VisualNode*> stack1;
  QStack<VisualNode*> stack2;

  /// The four needed for extracting labels
  NodeAllocator* _na1;
  NodeAllocator* _na2;

  Data* _data1;
  Data* _data2;

  /// The stack used while building new_tc
  QStack<VisualNode*> stack;

private: /// methods

  /// Initialize data source for obtaining labels etc.
  void setSource(NodeAllocator* na1, NodeAllocator* na2, Data* data1, Data* data2);

  /// Return true/false depending on whether n1 ~ n2
  bool copmareNodes(VisualNode* n1, VisualNode* n2); /// TODO: make it inline?

  /// 'which' is treated as a colour, usually 1 or 2 depending on which tree is a source
  unsigned int copyTree(VisualNode*, TreeCanvas*, 
                       VisualNode*, TreeCanvas*, int which = 0);

};

#endif

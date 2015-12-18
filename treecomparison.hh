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

#ifndef TREE_COMPARISON
#define TREE_COMPARISON

#include "treecanvas.hh"
#include "data.hh"

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

  Execution* _ex1;
  Execution* _ex2;

  /// The stack used while building new_tc
  QStack<VisualNode*> stack;

private: /// methods

  /// Initialize data source for obtaining labels etc.
  void setSource(NodeAllocator* na1, NodeAllocator* na2, Execution* ex1, Execution* ex2);

  /// Return true/false depending on whether n1 ~ n2
  bool copmareNodes(VisualNode* n1, VisualNode* n2); /// TODO: make it inline?

  /// 'which' is treated as a colour, usually 1 or 2 depending on which tree is a source
  unsigned int copyTree(VisualNode*, TreeCanvas*, 
                       VisualNode*, TreeCanvas*, int which = 0);

};

#endif

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

#include <QStack>
#include <vector>
#include <utility>
#include <unordered_map>

class TreeCanvas;
class VisualNode;
class Node;
class NodeAllocator;
class Execution;

struct PentagonItem {
  int l_size; /// left subtree size
  int r_size; /// right subtree size
  VisualNode* node; /// pentagon node
  const std::string* info; /// pentagon info -> now used for nogoods
  PentagonItem(int l_size, int r_size, VisualNode* node, const std::string* info = nullptr)
    :l_size(l_size), r_size(r_size), node(node), info(info) {}
};

class TreeComparison {


public:
  TreeComparison(Execution& ex1, Execution& ex2, bool withLabels);
  void compare(TreeCanvas* new_tc);
  void sortPentagons();

  const Execution& left_execution() const { return _ex1; }
  const Execution& right_execution() const { return _ex2; }

  int get_no_pentagons(void);

  /// NOTE(maxim): if use a pointer to an item here and vector is modified -> could be a problem
  const std::vector<PentagonItem>& pentagon_items() const { return _pentagon_items; }
  const std::unordered_map<int, int>& responsible_nogood_counts() const {
    return _responsible_nogood_counts;
  }

private:
  std::vector<PentagonItem> _pentagon_items;
  std::unordered_map<int, int> _responsible_nogood_counts;

private:

  /// The four needed for extracting labels
  Execution& _ex1;
  Execution& _ex2;

  NodeAllocator& _na1;
  NodeAllocator& _na2;

  bool withLabels_;

private: /// methods

  void analyseNogoods(const std::string& info);

  /// Returns true/false depending on whether n1 ~ n2
  bool copmareNodes(VisualNode* n1, VisualNode* n2);

  /// 'which' is treated as a colour, usually 1 or 2 depending on which tree is a source
  int copyTree(VisualNode*, TreeCanvas*,
               VisualNode*, const Execution&, int which = 0);

};

#endif

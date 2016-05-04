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
  int l_size;               /// left subtree size
  int r_size;               /// right subtree size
  VisualNode* node;         /// pentagon node
  const std::string* info;  /// pentagon info -> now used for nogoods
  PentagonItem(int l_size, int r_size, VisualNode* node,
               const std::string* info = nullptr)
      : l_size(l_size), r_size(r_size), node(node), info(info) {}
};

struct NogoodCmpStats {
  int occurrence;
  int search_eliminated;
};

class TreeComparison {
 public:
  TreeComparison(Execution& ex1, Execution& ex2);
  void compare(TreeCanvas* new_tc, bool with_labels);
  void sortPentagons();

  const Execution& left_execution() const { return _ex1; }
  const Execution& right_execution() const { return _ex2; }

  int get_no_pentagons();
  int get_total_reduced() const { return m_totalReduced; }

  /// NOTE(maxim): if use a pointer to an item here and vector is modified ->
  /// could be a problem
  const std::vector<PentagonItem>& pentagon_items() const {
    return m_pentagonItems;
  }
  const std::unordered_map<int, NogoodCmpStats>& responsible_nogood_stats()
      const {
    return m_responsibleNogoodStats;
  }

 private:
  /// aggregate comparison stats
  struct {
    int m_totalReduced = 0;
  };

  std::vector<PentagonItem> m_pentagonItems;
  std::unordered_map<int, NogoodCmpStats> m_responsibleNogoodStats;

  /// The four needed for extracting labels
  const Execution& _ex1;
  const Execution& _ex2;

  const NodeAllocator& _na1;
  const NodeAllocator& _na2;

private: /// methods
 void analyseNogoods(const std::string& info, int search_reduction);

 /// Returns true/false depending on whether n1 ~ n2
 bool copmareNodes(const VisualNode* n1, const VisualNode* n2,
                   bool with_labels);

 /// 'which' is treated as a colour, usually 1 or 2 depending on which tree is a
 /// source
 int copyTree(VisualNode*, TreeCanvas*, const VisualNode*, const Execution&,
              int which = 0);
};

#endif

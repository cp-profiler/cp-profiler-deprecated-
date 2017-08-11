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

#include <vector>
#include <unordered_map>

class Execution;
class TreeCanvas;
class VisualNode;
class Node;
class NodeAllocator;
class ComparisonResult;

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

namespace treecomparison {
std::unique_ptr<ComparisonResult> compareTrees(TreeCanvas& new_tc,
                                               const Execution& ex1,
                                               const Execution& ex2,
                                               bool with_labels);

std::unique_ptr<ComparisonResult> compareBinaryTrees(TreeCanvas& new_tc,
                                               const Execution& ex1,
                                               const Execution& ex2,
                                               bool with_labels);
}

class ComparisonResult {
  friend std::unique_ptr<ComparisonResult> treecomparison::compareTrees(
      TreeCanvas& new_tc, const Execution& ex1, const Execution& ex2,
      bool with_labels);

  friend std::unique_ptr<ComparisonResult> treecomparison::compareBinaryTrees(
      TreeCanvas& new_tc, const Execution& ex1, const Execution& ex2,
      bool with_labels);

  std::vector<PentagonItem> m_pentagonItems;
  std::unordered_map<int64_t, NogoodCmpStats> m_responsibleNogoodStats;
  int m_totalReduced = 0;
  const Execution& _ex1;
  const Execution& _ex2;

 public:
  ComparisonResult(const Execution& ex1, const Execution& ex2)
      : _ex1{ex1}, _ex2{ex2} {};

  void analyseNogoods(const std::string& info, int search_reduction);
  void sortPentagons();

  const std::vector<PentagonItem>& pentagon_items() const {
    return m_pentagonItems;
  }

  const std::unordered_map<int64_t, NogoodCmpStats>& responsible_nogood_stats()
      const {
    return m_responsibleNogoodStats;
  }

  int get_no_pentagons() const {
    return static_cast<int>(m_pentagonItems.size());
  }

  int get_total_reduced() const { return m_totalReduced; }

  const Execution& left_execution() const { return _ex1; }
  const Execution& right_execution() const { return _ex2; }
};

#endif

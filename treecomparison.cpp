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

#include <QStack>
#include "execution.hh"
#include "treecomparison.hh"
#include "treecanvas.hh"
#include "node.hh"
#include "data.hh"

using std::string;

/// Copy the structure (and status/connects to data) from one node/execution into another;
/// 'which' is treated as a colour (1 or 2 depending on which tree is a source)
static int copyTree(VisualNode* target_node, Execution& ex_target,
              const VisualNode* root, const Execution& ex_source,
              int which) {

    NodeTree& target_tree = ex_target.nodeTree();
    const NodeTree& source_tree = ex_source.nodeTree();

    NodeAllocator& na = target_tree.getNA();
    const NodeAllocator& na_source = source_tree.getNA();

    QStack<const VisualNode*> source_stack;
    QStack<VisualNode*> target_stack;

    source_stack.push(root);
    target_stack.push(target_node);

    int count = 0;

    while (source_stack.size() > 0) {
        count++;

        const VisualNode* n = source_stack.pop();
        VisualNode* next = target_stack.pop();

        next->_tid = which; // treated as a colour

        uint kids = n->getNumberOfChildren();
        next->setNumberOfChildren(kids, na);
        // next->setStatus(n->getStatus());
        next->nstatus = n->nstatus;

        /// point to the source node

        if (n->getStatus() != NodeStatus::UNDETERMINED) {
            auto& source_data = ex_source.getData();

            int source_index = source_tree.getIndex(n);
            DbEntry* entry = source_data.getEntry(source_index);

            auto& this_data = ex_target.getData();
            int target_index = target_tree.getIndex(next);
            this_data.connectNodeToEntry(target_index, entry);

            /// TODO(maxim): connect nogoods as well

            auto sid = entry->s_node_id;
            auto info = source_data.sid2info.find(sid);

            /// note(maxim): should have to maintain another map
            /// (even though info is only a pointer)
            if (info != source_data.sid2info.end()) {
                this_data.sid2info[sid] = info->second;
            }

        }

        next->dirtyUp(na);

        for (uint i = 0; i < kids; ++i) {
            source_stack.push(n->getChild(na_source, i));
            target_stack.push(next->getChild(na, i));
        }
    }

    return count;
}

static void find_and_replace_all(std::string& str, std::string substr_old, std::string substr_new) {
    auto pos = str.find(substr_old);
    while (pos != std::string::npos) {
        str.replace(pos, std::string(substr_old).length(), substr_new);
        pos = str.find(substr_old);
    }
}


/// TODO(maxim): this is broken!!!
static bool compareLabels(std::string lhs, std::string rhs) {
    /// NOTE(maxim): removes whitespaces before comparing;
    /// this will be necessary as long as Chuffed and Gecode don't agree
    /// on whether to put whitespaces around operators (Gecode uses ' '
    /// for parsing logbrancher while Chuffed uses them as a delimiter
    /// between literals)

    lhs.erase(remove_if(lhs.begin(), lhs.end(), isspace), lhs.end());
    rhs.erase(remove_if(rhs.begin(), rhs.end(), isspace), rhs.end());

    find_and_replace_all(lhs, "==", "=");
    find_and_replace_all(rhs, "==", "=");

    // qDebug() << "comparing: " << lhs.c_str() << " " << rhs.c_str();

    if (lhs.compare(rhs) != 0) {
#ifdef MAXIM_DEBUG
      qDebug() << "labels not equal: " << lhs.c_str() << " " << rhs.c_str();
#endif
      return false;
    }

    return true;
}

/// Returns true/false depending on whether n1 ~ n2
static bool copmareNodes(const VisualNode* n1, const Execution& ex1,
                  const VisualNode* n2, const Execution& ex2,
                  bool with_labels) {
  const auto kids1 = n1->getNumberOfChildren();
  const auto kids2 = n2->getNumberOfChildren();
  if (kids1 != kids2) return false;

  if (n1->getStatus() != n2->getStatus()) return false;

  /// check labels
  // if (with_labels) {
  //   for (auto i = 0u; i < kids1; i++) {
  //     int id1 = n1->getChild(i);
  //     int id2 = n2->getChild(i);

  //     auto label1 = ex1.getLabel(id1);
  //     auto label2 = ex2.getLabel(id2);

  //     if (!compareLabels(label1, label2)) return false;

  //   }
  // }

  /// check your own labels only, not children's
  if (with_labels) {
    for (auto i = 0u; i < kids1; i++) {

      auto label1 = ex1.getLabel(*n1);
      auto label2 = ex2.getLabel(*n2);

      if (!compareLabels(label1, label2)) return false;

    }
  }

  return true;
}

void ComparisonResult::sortPentagons() {
    std::sort(m_pentagonItems.begin(), m_pentagonItems.end(),
        [](const PentagonItem& lhs, const PentagonItem& rhs){
            return std::abs((int)lhs.l_size - (int)lhs.r_size)
                 > std::abs((int)rhs.l_size - (int)rhs.r_size);
    });
}

/// note(maxim): Defined in cmp_tree_dialog.cpp
std::vector<int>
infoToNogoodVector(const string& info);

void ComparisonResult::analyseNogoods(const string& info, int search_reduction) {
    auto ng_vec = infoToNogoodVector(info);

    for (auto ng_id : ng_vec) {
        auto& ng_stats = m_responsibleNogoodStats[ng_id];
        ng_stats.occurrence++;
        ng_stats.search_eliminated += search_reduction / ng_vec.size();
    }

    m_totalReduced += search_reduction;

}

namespace treecomparison {


std::unique_ptr<ComparisonResult> compare(TreeCanvas* new_tc,
                                          const Execution& ex1,
                                          const Execution& ex2,
                                          bool with_labels) {
  auto& na1 = ex1.nodeTree().getNA();
  auto& na2 = ex2.nodeTree().getNA();

  std::unique_ptr<ComparisonResult> result(new ComparisonResult{ex1, ex2});

  /// For source trees
  QStack<const VisualNode*> stack1;
  QStack<const VisualNode*> stack2;
  /// The stack used while building new_tc
  QStack<VisualNode*> stack;

  const VisualNode* root1 = ex1.nodeTree().getRoot();
  const VisualNode* root2 = ex2.nodeTree().getRoot();

  VisualNode* next;

  stack1.push(root1);
  stack2.push(root2);

  Execution& execution = *new_tc->getExecution();

  VisualNode* root = execution.nodeTree().getRoot();
  stack.push(root);

  bool rootBuilt = false;

  NodeAllocator& na = execution.nodeTree().getNA();

  while (stack1.size() > 0) {
    auto node1 = stack1.pop();
    auto node2 = stack2.pop();

    /// ---------- Skipping implied ---------------

    int implied_child;

    /// check if any children of node 1 are implied
    /// if so, skip this node (stack1.pop())

    do {
      implied_child = -1;

      auto kids = node1->getNumberOfChildren();
      for (auto i = 0u; i < kids; i++) {
        std::string label = ex1.getLabel(node1->getChild(i));

        /// check if label starts with "[i]"

        if (label.compare(0, 3, "[i]") == 0) {
          implied_child = i;
          break;
          qDebug() << "found implied: " << label.c_str();
        }
      }

      /// if implied not found -> continue,
      /// otherwise skip this node
      if (implied_child != -1) {
        node1 = node1->getChild(na1, implied_child);
      }
    } while (implied_child != -1);

    /// the same for node 2

    do {
      implied_child = -1;

      auto kids = node2->getNumberOfChildren();
      for (auto i = 0u; i < kids; i++) {
        std::string label = ex2.getLabel(node2->getChild(i));

        /// check if label starts with "[i]"

        if (label.compare(0, 3, "[i]") == 0) {
          implied_child = i;
          break;
          qDebug() << "found implied: " << label.c_str();
        }
      }

      /// if implied not found -> continue,
      /// otherwise skip this node
      if (implied_child != -1) {
        node2 = node2->getChild(na2, implied_child);
      }
    } while (implied_child != -1);

    /// ----------------------------------------------------

    bool equal = copmareNodes(node1, ex1, node2, ex2, with_labels);

    if (equal) {
      auto kids = node1->getNumberOfChildren();
      for (auto i = 0u; i < kids; ++i) {
        stack1.push(node1->getChild(na1, kids - i - 1));
        stack2.push(node2->getChild(na2, kids - i - 1));
      }

      /// if roots are equal
      if (!rootBuilt) {
        next = na[0];
        rootBuilt = true;
      } else {
        next = stack.pop();
      }

      /// new node is built

      next->setNumberOfChildren(kids, na);
      next->nstatus = node1->nstatus;
      next->_tid = 0;

      /// point to the source node

      auto source_index = node2->getIndex(na2);
      auto target_index = next->getIndex(na);

      auto entry = ex2.getEntry(source_index);
      execution.getData().connectNodeToEntry(target_index, entry);

      for (auto i = 0u; i < kids; ++i) {
        stack.push(next->getChild(na, kids - i - 1));
      }

    } else {
      /// not equal
      // qDebug() << "nodes are not equal";
      next = stack.pop();
      next->setNumberOfChildren(2, na);
      next->setStatus(MERGING);
      if (!next->isRoot()) next->getParent(na)->setHidden(false);
      next->setHidden(true);
      next->_tid = 0;

      new_tc->unhideNode(next);  /// unhide pentagons if hidden

      stack.push(next->getChild(na, 1));
      stack.push(next->getChild(na, 0));

      int left_size =
          copyTree(stack.pop(), *new_tc->getExecution(), node1, ex1, 1);
      int right_size =
          copyTree(stack.pop(), *new_tc->getExecution(), node2, ex2, 2);

      /// hide the nodes one level below a pentagon
      /// if they are hidden on the original tree
      assert(next->getNumberOfChildren() == 2);

      if (node1->getNumberOfChildren() > 0 &&
          !node1->isNodeVisible(ex1.nodeTree().getNA())) {
        next->getChild(na, 0)->setHidden(true);
      }

      if (node2->getNumberOfChildren() > 2 &&
          !node2->isNodeVisible(ex2.nodeTree().getNA())) {
        next->getChild(na, 1)->setHidden(true);
      }

      const string* info_str = nullptr;
      /// if node1 is FAILED -> check nogoods // TODO(maxim): branch node?
      if (node1->getStatus() == FAILED) {
        info_str = ex1.getInfo(*node1);

        int search_reduction = right_size - left_size;
        /// identify nogoods and increment counters
        if (info_str) {
          result->analyseNogoods(*info_str, search_reduction);
        }
      }

      result->m_pentagonItems.emplace_back(
          PentagonItem{left_size, right_size, next, info_str});
    }

    // qDebug() << "comparison na: " << (void*)(&na);

    next->dirtyUp(na);
    new_tc->update();
  }

  return result;
}

}

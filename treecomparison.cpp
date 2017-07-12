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
#include "nodetree.hh"
#include "cpprofiler/utils/tree_utils.hh"

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

            auto sid = entry->full_sid;
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

/// Returns true if n1 ~ n2
static bool copmareNodes(const VisualNode* n1, const Execution& ex1,
                  const VisualNode* n2, const Execution& ex2,
                  bool with_labels) {

  /// if one is a nullptr -> not equal
  if (!n1 || !n2) return false;

  // Two nodes are euqal if:
  //    have the same status
  //    have the same labels

  if (n1->getStatus() != n2->getStatus()) return false;

  const auto kids1 = n1->getNumberOfChildren();
  const auto kids2 = n2->getNumberOfChildren();

  /// check your own labels only, not children's
  if (with_labels) {
      const auto& label1 = ex1.getLabel(*n1);
      const auto& label2 = ex2.getLabel(*n2);
      if (!compareLabels(label1, label2)) return false;
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

static const VisualNode* skip_implied(const Execution& ex, const VisualNode* node) {

  if (!node) return node;

  const auto& na = ex.nodeTree().getNA();

  int implied_child;

  do {
    implied_child = -1;

    const auto kids = node->getNumberOfChildren();
    for (auto i = 0u; i < kids; i++) {
      const auto& label = ex.getLabel(node->getChild(i));

      /// check if label starts with "[i]"
      if (label.compare(0, 3, "[i]") == 0) {
        implied_child = i;
        break;
      }
    }

    /// if implied found, skip this node
    if (implied_child != -1) {
      node = node->getChild(na, implied_child);
    }
  } while (implied_child != -1);

  return node;
}


/// do whatever it means for a `target` node to represent `source`
static void copy_into(const Execution& s_ex, const VisualNode* source,
                      const Execution& t_ex, VisualNode* target) {
  /// copy status and various flags (not sure if all needed)
  target->nstatus = source->nstatus;

  auto& s_na = s_ex.nodeTree().getNA();
  auto& t_na = t_ex.nodeTree().getNA();

  auto s_idx = source->getIndex(s_na);
  auto t_idx = target->getIndex(t_na);

  auto entry = s_ex.getEntry(s_idx);
  t_ex.getData().connectNodeToEntry(t_idx, entry);

}

static std::pair<int,int> create_pentagon(Execution& t_ex, VisualNode* target,
                            const Execution& ex1, const VisualNode* n1,
                            const Execution& ex2, const VisualNode* n2) {

  auto& t_na = t_ex.nodeTree().getNA();

  target->setStatus(MERGING);

  utils::unhideFromNodeToRoot(t_ex.nodeTree(), *target);
  target->setNumberOfChildren(2, t_na);

  auto left = target->getChild(t_na, 0);
  auto right = target->getChild(t_na, 1);

  int left_size;
  int right_size;

  if (n1) {
    left_size = copyTree(left, t_ex, n1, ex1, 1);

    /// check if need to hide n1:
    auto& na_1 = ex1.nodeTree().getNA();
    if (n1->getNumberOfChildren() > 0 && !n1->isNodeVisible(na_1)) {
      left->setHidden(true);
    }

  } else {
    left->setInvisible(true);
  }

  if (n2) {
    right_size = copyTree(right, t_ex, n2, ex2, 2);

    /// check if need to hide n1:
    auto& na_2 = ex2.nodeTree().getNA();
    if (n2->getNumberOfChildren() > 0 && !n2->isNodeVisible(na_2)) {
      right->setHidden(true);
    }

  } else {
    right->setInvisible(true);
  }

  return std::make_pair(left_size, right_size);

}

std::unique_ptr<ComparisonResult> compareTrees(TreeCanvas& new_tc,
                                               const Execution& ex1,
                                               const Execution& ex2,
                                               bool with_labels) {

  /// For source trees
  QStack<const VisualNode*> stack1, stack2;

  /// The stack used for building new_tc
  QStack<VisualNode*> stack;

  auto root1 = ex1.nodeTree().getRoot();
  auto root2 = ex2.nodeTree().getRoot();

  auto& na1 = ex1.nodeTree().getNA();
  auto& na2 = ex2.nodeTree().getNA();

  stack1.push(root1); stack2.push(root2);

  auto& ex = new_tc.getExecution();
  auto& nt = ex.nodeTree();
  auto& na = nt.getNA();

  stack.push(nt.getRoot());

  std::unique_ptr<ComparisonResult> result(new ComparisonResult{ex1, ex2});

  while (stack1.size() > 0) {

    auto node1 = stack1.pop();
    auto node2 = stack2.pop();
    auto target = stack.pop();

    /// TODO: check if implied

    bool equal = copmareNodes(node1, ex1, node2, ex2, with_labels);


    if (equal) {
      /// turn current node into one of node1/node2

      auto kids1 = (int)node1->getNumberOfChildren();
      auto kids2 = (int)node2->getNumberOfChildren();

      auto min_kids = std::min(kids1, kids2);
      auto diff_kids = std::abs(kids1 - kids2);

      target->setNumberOfChildren(min_kids + diff_kids, na);

      copy_into(ex1, node1, ex, target);

      for (auto i = 0u; i < min_kids; i++) {
        stack1.push(node1->getChild(na1, min_kids - i - 1));
        stack2.push(node2->getChild(na2, min_kids - i - 1));
        stack.push(target->getChild(na, min_kids - i - 1));
      }

      for (auto i = min_kids; i < min_kids + diff_kids; i++) {
        auto child = target->getChild(na, i);

        if (kids1 > kids2) {
          auto node = node1->getChild(na1, i);

          if (node->getStatus() == UNDETERMINED || node->getStatus() == SKIPPED) {
            continue;
          }
          auto ignored = create_pentagon(ex, child, ex1, node, ex2, nullptr);
        } else {
          auto node = node2->getChild(na2, i);

          if (node->getStatus() == UNDETERMINED  || node->getStatus() == SKIPPED) {
            continue;
          }
          auto ignored = create_pentagon(ex, child, ex1, nullptr, ex2, node);
        }
      }

      /// TODO: crate pentagons here as well

    } else {
      int left_size, right_size;
      std::tie(left_size, right_size) = create_pentagon(ex, target, ex1, node1, ex2, node2);

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
        PentagonItem{left_size, right_size, target, info_str});
    }

    target->dirtyUp(na);

    new_tc.updateCanvas();

  }

  return result;
}

}

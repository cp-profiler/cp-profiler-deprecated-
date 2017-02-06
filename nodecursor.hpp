/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Guido Tack, 2006
 *
 *  Last modified:
 *     $Date$ by $Author$
 *     $Revision$
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "treecanvas.hh"

inline
NodeCursor::NodeCursor(VisualNode* theNode,
                             const NodeAllocator& na0)
    : _startNode(theNode), _node(theNode),
      _alternative(theNode->getAlternative(na0)),
      na(na0) {}

inline VisualNode*
NodeCursor::node(void) { return _node; }

inline const VisualNode*
NodeCursor::node(void) const { return _node; }

inline unsigned int
NodeCursor::alternative(void) const { return _alternative; }

inline void
NodeCursor::alternative(unsigned int a) { _alternative=a; }

inline VisualNode*
NodeCursor::startNode(void) { return _startNode; }

inline const VisualNode*
NodeCursor::startNode(void) const { return _startNode; }

inline void
NodeCursor::node(VisualNode* n) { _node = n; }


inline bool
NodeCursor::mayMoveUpwards(void) const {
    return _node != _startNode && !_node->isRoot();
}

inline void
NodeCursor::moveUpwards(void) {
    _node = static_cast<VisualNode*>(_node->getParent(na));
    if (_node->isRoot()) {
        _alternative = 0;
    } else {
        VisualNode* p = static_cast<VisualNode*>(_node->getParent(na));
        for (int i=p->getNumberOfChildren(); i--;) {
            if (p->getChild(na,i) == _node) {
                _alternative = i;
                break;
            }
        }
    }
}

inline bool
NodeCursor::mayMoveDownwards(void) const {
    int n = _node->getNumberOfChildren();
    return (n > 0);
}

inline void
NodeCursor::moveDownwards(void) {
    _alternative = 0;
    _node = _node->getChild(na,0);
}

inline bool
NodeCursor::mayMoveSidewards(void) const {
    return (!_node->isRoot()) && (_node != _startNode) &&
            (_alternative < _node->getParent(na)->getNumberOfChildren() - 1);
}

inline void
NodeCursor::moveSidewards(void) {
    _node =
            static_cast<VisualNode*>(_node->getParent(na)->getChild(na,++_alternative));
}

inline bool
HideFailedCursor::mayMoveDownwards(void) const {

    const VisualNode* n = node();

    return (!onlyDirty || n->isDirty()) &&
            NodeCursor::mayMoveDownwards() &&
            (n->hasSolvedChildren() || n->getNoOfOpenChildren(na) > 0) &&
            (! n->isHidden());
}

inline
HideFailedCursor::HideFailedCursor(VisualNode* root,
                                   const NodeAllocator& na,
                                   bool onlyDirtyNodes)
    : NodeCursor(root,na), onlyDirty(onlyDirtyNodes) {}

inline void
HideFailedCursor::processCurrentNode(void) {

    VisualNode* n = node();
    // std::cerr << "HideFailedCursor::processCurrentNode: " << n->getStatus() << " " << n->hasSolvedChildren() << " " << n->getNoOfOpenChildren(na) << "\n";
    if (n->getStatus() == BRANCH &&
            !n->hasSolvedChildren() &&
            n->getNoOfOpenChildren(na) == 0) {
        n->setHidden(true);
        n->setChildrenLayoutDone(false);
        n->dirtyUp(na);
    }
}

inline
UnhideAllCursor::UnhideAllCursor(VisualNode* root,
                                 const NodeAllocator& na)
    : NodeCursor(root,na) {}

inline void
UnhideAllCursor::processCurrentNode(void) {
    VisualNode* n = node();
    if (n->isHidden()) {
        n->setHidden(false);
        n->dirtyUp(na);
    }
}

inline
UnselectAllCursor::UnselectAllCursor(VisualNode* root,
                                     const NodeAllocator& na)
    : NodeCursor(root,na) {}

inline void
UnselectAllCursor::processCurrentNode(void) {
    VisualNode* n = node();
    if (n->isSelected()) {
        n->setSelected(false);
        n->dirtyUp(na);
    }
}


inline
HideAllCursor::HideAllCursor(VisualNode* root,
                                 const NodeAllocator& na)
    : NodeCursor(root,na) {}

inline void
HideAllCursor::processCurrentNode(void) {
    VisualNode* n = node();
    if (!n->isHidden()) {
        n->setHidden(true);
        n->dirtyUp(na);
    }
}

inline
NextSolCursor::NextSolCursor(VisualNode* theNode, bool backwards,
                             const NodeAllocator& na)
    : NodeCursor(theNode,na), back(backwards) {}

inline void
NextSolCursor::processCurrentNode(void) {}

inline bool
NextSolCursor::notOnSol(void) const {
    return node() == startNode() || node()->getStatus() != SOLVED;
}

inline bool
NextSolCursor::mayMoveUpwards(void) const {
    return notOnSol() && !node()->isRoot();
}

inline bool
NextSolCursor::mayMoveDownwards(void) const {
    return notOnSol() && !(back && node() == startNode())
            && node()->hasSolvedChildren()
            && NodeCursor::mayMoveDownwards();
}

inline void
NextSolCursor::moveDownwards(void) {
    NodeCursor::moveDownwards();
    if (back) {
        while (NodeCursor::mayMoveSidewards())
            NodeCursor::moveSidewards();
    }
}

inline bool
NextSolCursor::mayMoveSidewards(void) const {
    if (back) {
        return notOnSol() && !node()->isRoot() && alternative() > 0;
    } else {
        return notOnSol() && !node()->isRoot() &&
                (alternative() <
                 node()->getParent(na)->getNumberOfChildren() - 1);
    }
}

inline void
NextSolCursor::moveSidewards(void) {
    if (back) {
        alternative(alternative()-1);
        node(node()->getParent(na)->getChild(na,alternative()));
    } else {
        NodeCursor::moveSidewards();
    }
}






inline
NextLeafCursor::NextLeafCursor(VisualNode* theNode, bool backwards,
                             const NodeAllocator& na)
    : NodeCursor(theNode,na), back(backwards) {}

inline void
NextLeafCursor::processCurrentNode(void) {}

inline bool
NextLeafCursor::notOnLeaf(void) const {
  if (node() == startNode()) return true;
  NodeStatus s = node()->getStatus();
  bool isLeaf = s == SOLVED || s == FAILED;
  return !isLeaf;
}

inline bool
NextLeafCursor::mayMoveUpwards(void) const {
    return notOnLeaf() && !node()->isRoot();
}

inline bool
NextLeafCursor::mayMoveDownwards(void) const {
    return notOnLeaf() && !(back && node() == startNode())
            && NodeCursor::mayMoveDownwards();
}

inline void
NextLeafCursor::moveDownwards(void) {
    NodeCursor::moveDownwards();
    if (back) {
        while (NodeCursor::mayMoveSidewards())
            NodeCursor::moveSidewards();
    }
}

inline bool
NextLeafCursor::mayMoveSidewards(void) const {
    if (back) {
        return notOnLeaf() && !node()->isRoot() && alternative() > 0;
    } else {
        return notOnLeaf() && !node()->isRoot() &&
                (alternative() <
                 node()->getParent(na)->getNumberOfChildren() - 1);
    }
}

inline void
NextLeafCursor::moveSidewards(void) {
    if (back) {
        alternative(alternative()-1);
        node(node()->getParent(na)->getChild(na,alternative()));
    } else {
        NodeCursor::moveSidewards();
    }
}


/// **************

inline
NextPentagonCursor::NextPentagonCursor(VisualNode* theNode, bool backwards,
                             const NodeAllocator& na)
    : NodeCursor(theNode,na), back(backwards) {}

inline void
NextPentagonCursor::processCurrentNode(void) {}

inline bool
NextPentagonCursor::notOnPentagon(void) const {
    return node() == startNode() || node()->getStatus() != MERGING;
}

inline bool
NextPentagonCursor::mayMoveUpwards(void) const {
    return notOnPentagon() && !node()->isRoot();
}

inline bool
NextPentagonCursor::mayMoveDownwards(void) const {
    return notOnPentagon() && !(back && node() == startNode())
            && NodeCursor::mayMoveDownwards();
}

inline void
NextPentagonCursor::moveDownwards(void) {
    NodeCursor::moveDownwards();
    if (back) {
        while (NodeCursor::mayMoveSidewards())
            NodeCursor::moveSidewards();
    }
}

inline bool
NextPentagonCursor::mayMoveSidewards(void) const {
    if (back) {
        return notOnPentagon() && !node()->isRoot() && alternative() > 0;
    } else {
        return notOnPentagon() && !node()->isRoot() &&
                (alternative() <
                 node()->getParent(na)->getNumberOfChildren() - 1);
    }
}

inline void
NextPentagonCursor::moveSidewards(void) {
    if (back) {
        alternative(alternative()-1);
        node(node()->getParent(na)->getChild(na,alternative()));
    } else {
        NodeCursor::moveSidewards();
    }
}



/// **************

inline
StatCursor::StatCursor(VisualNode* root,
                       const NodeAllocator& na)
    : NodeCursor(root,na),
      curDepth(0), depth(0), failed(0), solved(0), choice(0), open(0) {}

inline void
StatCursor::processCurrentNode(void) {
    VisualNode* n = node();
    switch (n->getStatus()) {
    case SOLVED: solved++; break;
    case FAILED: failed++; break;
    case BRANCH: choice++; break;
    case UNDETERMINED: open++; break;
    default: break;
    }
}

inline void
StatCursor::moveDownwards(void) {
    curDepth++;
    depth = std::max(depth,curDepth);
    NodeCursor::moveDownwards();
}

inline void
StatCursor::moveUpwards(void) {
    curDepth--;
    NodeCursor::moveUpwards();
}

inline
HighlightCursor::HighlightCursor(VisualNode* startNode,
  const NodeAllocator& na)
: NodeCursor(startNode, na) {}

inline void
HighlightCursor::processCurrentNode(void) {
  VisualNode* n = node();
  n->setHighlighted(true);
}

inline
UnhighlightCursor::UnhighlightCursor(VisualNode* root,
  const NodeAllocator& na)
: NodeCursor(root, na) {}

inline void
UnhighlightCursor::processCurrentNode(void) {
  VisualNode* n = node();
  n->setHighlighted(false);
}

inline
CountSolvedCursor::CountSolvedCursor(VisualNode* startNode,
  const NodeAllocator& na, int &count)
: NodeCursor(startNode, na), _count(count){
  _count = 0;
}

inline void
CountSolvedCursor::processCurrentNode(void) {
  const VisualNode* n = node();
  if (n->getStatus() == SOLVED){
    _count++;
  }
}

inline
GetIndexesCursor::GetIndexesCursor(VisualNode* startNode,
  const NodeAllocator& na, std::vector<int>& node_gids)
: NodeCursor(startNode, na), _na(na), _node_gids(node_gids){

}

inline void
GetIndexesCursor::processCurrentNode(void) {
  VisualNode* n = node();
  _node_gids.push_back(n->getIndex(_na));
}

inline
HideNotHighlightedCursor::HideNotHighlightedCursor(VisualNode* startNode,
  const NodeAllocator& na)
  : NodeCursor(startNode,na) {}

inline void
HideNotHighlightedCursor::processCurrentNode(void) {
  VisualNode* n = node();
  bool onHP = n->isHighlighted();
  if (!onHP) {
    for (int i=n->getNumberOfChildren(); i--;) {
      if (onHighlightPath.contains(n->getChild(na,i))) {
        onHP = true;
        break;
      }
    }
  }
  if (onHP) {
    onHighlightPath[n] = true;
  } else {
    n->setHidden(true);
    n->setChildrenLayoutDone(false);
    n->dirtyUp(na);
  }
}

inline bool
HideNotHighlightedCursor::mayMoveDownwards(void) const {
  return NodeCursor::mayMoveDownwards() &&
    (!node()->isHighlighted()) &&
    (!node()->isHidden());
}


inline
BranchLabelCursor::BranchLabelCursor(VisualNode* root, bool clear,
    NodeAllocator& na, TreeCanvas& tc)
    : NodeCursor(root,na), _na(na), _tc(tc), _clear(clear) {}

inline void
BranchLabelCursor::processCurrentNode(void) {
    VisualNode* n = node();
    if (!_clear) {
        if (!na.hasLabel(n)) {
// #ifdef MAXIM_DEBUG
//             _na.setLabel(n, " " + QString::number(n->debug_id) + " ");
//             n->dirtyUp(na);
//             return;
// #endif
            VisualNode* p = n->getParent(_na);
            if (p) {
                int gid = n->getIndex(_na);
                std::string l = _tc.getLabel(gid);
                _na.setLabel(n,QString(l.c_str()));
            } else {
                _na.setLabel(n,"");
            }
        }
    } else {
        _na.clearLabel(n);
    }
    n->dirtyUp(na);
}

inline
DisposeCursor::DisposeCursor(VisualNode* root,
                             const NodeAllocator& na)
    : NodeCursor(root,na) {}

inline void
DisposeCursor::processCurrentNode(void) {
    node()->dispose();
}

inline
SubtreeCountCursor::SubtreeCountCursor(VisualNode *theNode,
                   int _threshold,
                   const NodeAllocator& na)
  : NodeCursor(theNode, na),
    threshold(_threshold)
{
    stack.push_back(0);
}

inline void
SubtreeCountCursor::processCurrentNode(void) {
    stack.back()++;
    int x = stack.back();
    VisualNode* n = node();
    // A threshold of zero means turn this stuff off.
    if (threshold == 0) {
        n->setSubtreeSizeUnknown();
        n->setHidden(false);
        n->setChildrenLayoutDone(false);
    } else if (1 < x && x <= threshold) {
        n->setHidden(true);
        n->setChildrenLayoutDone(true);
        if (x < threshold/4) {
            n->setSubtreeSize(0);
        } else if (x < threshold/2) {
            n->setSubtreeSize(1);
        } else if (x < 3*threshold/4) {
            n->setSubtreeSize(2);
        } else {
            n->setSubtreeSize(3);
        }
    } else {
        n->setHidden(false);
        n->setChildrenLayoutDone(false);
    }
    n->dirtyUp(na);
}

inline void
SubtreeCountCursor::moveSidewards(void) {
    NodeCursor::moveSidewards();
    int x = stack.back();
    stack.pop_back();
    stack.back() += x;
    stack.push_back(0);
}

inline void
SubtreeCountCursor::moveUpwards(void) {
    NodeCursor::moveUpwards();
    int x = stack.back();
    stack.pop_back();
    stack.back() += x;
}

inline void
SubtreeCountCursor::moveDownwards(void) {
    NodeCursor::moveDownwards();
    stack.push_back(0);
}



inline
SearchLogCursor::SearchLogCursor(VisualNode *theNode,
                                 std::stringstream& outputStream,
                                 const NodeAllocator& na,
                                 const Execution& execution)
    : NodeCursor(theNode, na),
      _out(outputStream),
      _na(na),
      _execution(execution)
{}

static std::string reverseOp(const char* op) {
  if (strcmp(op, "=") == 0) { return "!="; }
  if (strcmp(op, "==") == 0) { return "!="; }
  if (strcmp(op, ">") == 0) { return "<="; }
  if (strcmp(op, "<") == 0) { return ">="; }
  if (strcmp(op, "<=") == 0) { return ">"; }
  if (strcmp(op, ">=") == 0) { return "<"; }

  std::cerr << "no such operator, aborting...\n";
  abort();
  return "";
}

static std::string reverseLabel(const std::string& str) {
  auto operators = {"<=", ">=", "!=", "==", "=", "<", ">"}; /// order matters here
  for (auto op : operators) {
    auto pos = str.find(op);
    if (pos == std::string::npos) continue;

    return str.substr(0, pos) + reverseOp(op) + str.substr(pos + strlen(op));
  }
  abort();
  return "";
}

inline void
SearchLogCursor::processCurrentNode(void) {
    VisualNode* n = node();
    int numChildren = n->getNumberOfChildren();

    int nonskipped_kids = 0;

    std::stringstream children_stream;

    for (int i = 0 ; i < numChildren ; i++) {

        VisualNode* child = n->getChild(_na, i);
        int child_gid = n->getChild(i);

        auto child_label = _execution.getLabel(child_gid);

        // ignore "skipped nodes"
        if (child->getStatus() == SKIPPED) continue;

        // Impossible as such nodes would have been deleted
        // assert(child_label != "");

        ++nonskipped_kids;

        // if (i == 1 && numChildren == 2 && child_label == "") {
        //   auto prev = _execution.getLabel(n->getChild(0));

        //   if (prev != "") {
        //     qDebug() << "reversing: " << prev.c_str();
        //     child_label = reverseLabel(prev);
        //   }
        // }

        // The child's index and its label.
        children_stream << " " << child_gid << " " << child_label.c_str();
    }

    _out << n->getIndex(_na) << " " << nonskipped_kids;

    // Unexplored node on the left branch (search timed out)
    if ((numChildren == 0) && (n->getStatus() == BRANCH)) {
      _out << " stop";
    }

    _out << children_stream.str().c_str();
    _out << "\n";
}

inline
UnhideAncestorsCursor::UnhideAncestorsCursor(VisualNode* root,
                                 const NodeAllocator& na)
    : NodeCursor(root,na) {}

inline void
UnhideAncestorsCursor::processCurrentNode(void) {
    VisualNode* n = node();
    if (n->isHidden()) {
        n->setHidden(false);
        n->dirtyUp(na);
    }
    // n->setChildrenLayoutDone(false);
}

inline bool
UnhideAncestorsCursor::mayMoveUpwards(void) const {
    return !node()->isRoot();
}

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

template<class Node>
inline
NodeCursor<Node>::NodeCursor(Node* theNode,
                             const NodeAllocator& na0)
    : _startNode(theNode), _node(theNode),
      _alternative(theNode->getAlternative(na0)),
      na(na0) {}

template<class Node>
inline Node*
NodeCursor<Node>::node(void) { return _node; }

template<class Node>
inline unsigned int
NodeCursor<Node>::alternative(void) { return _alternative; }

template<class Node>
inline void
NodeCursor<Node>::alternative(unsigned int a) { _alternative=a; }

template<class Node>
inline Node*
NodeCursor<Node>::startNode(void) { return _startNode; }

template<class Node>
inline void
NodeCursor<Node>::node(Node* n) { _node = n; }

template<class Node>
inline bool
NodeCursor<Node>::mayMoveUpwards(void) {
    return _node != _startNode && !_node->isRoot();
}

template<class Node>
inline void
NodeCursor<Node>::moveUpwards(void) {
    _node = static_cast<Node*>(_node->getParent(na));
    if (_node->isRoot()) {
        _alternative = 0;
    } else {
        Node* p = static_cast<Node*>(_node->getParent(na));
        for (int i=p->getNumberOfChildren(); i--;) {
            if (p->getChild(na,i) == _node) {
                _alternative = i;
                break;
            }
        }
    }
}

template<class Node>
inline bool
NodeCursor<Node>::mayMoveDownwards(void) {
    int n = _node->getNumberOfChildren();
    return (n > 0);
}

template<class Node>
inline void
NodeCursor<Node>::moveDownwards(void) {
    _alternative = 0;
    _node = _node->getChild(na,0);
}

template<class Node>
inline bool
NodeCursor<Node>::mayMoveSidewards(void) {
    return (!_node->isRoot()) && (_node != _startNode) &&
            (_alternative < _node->getParent(na)->getNumberOfChildren() - 1);
}

template<class Node>
inline void
NodeCursor<Node>::moveSidewards(void) {
    _node =
            static_cast<Node*>(_node->getParent(na)->getChild(na,++_alternative));
}

inline bool
HideFailedCursor::mayMoveDownwards(void) {

    // std::cerr << "--- mayMoveDownwards ---\n";

    VisualNode* n = node();

    // std::cerr << "id: " << n->debug_id;

    // std::cerr << "   isDirty: " << n->isDirty() << "\n";
    // std::cerr << "   mayMoveDownwards: " << NodeCursor<VisualNode>::mayMoveDownwards() << "\n";
    // std::cerr << "   hasSolvedChildren: " << n->hasSolvedChildren() << "\n";
    // std::cerr << "   number of open kids: " << n->getNoOfOpenChildren(na) << "\n";
    // std::cerr << "   isHidden: " << n->isHidden() << "\n";
    // std::cerr << "------------------------\n";
    return (!onlyDirty || n->isDirty()) &&
            NodeCursor<VisualNode>::mayMoveDownwards() &&
            (n->hasSolvedChildren() || n->getNoOfOpenChildren(na) > 0) &&
            (! n->isHidden());
}

inline
HideFailedCursor::HideFailedCursor(VisualNode* root,
                                   const NodeAllocator& na,
                                   bool onlyDirtyNodes)
    : NodeCursor<VisualNode>(root,na), onlyDirty(onlyDirtyNodes) {}

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
    : NodeCursor<VisualNode>(root,na) {}

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
    : NodeCursor<VisualNode>(root,na) {}

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
    : NodeCursor<VisualNode>(root,na) {}

inline void
HideAllCursor::processCurrentNode(void) {
    VisualNode* n = node();
    if (!n->isHidden()) {
        n->setHidden(true);
        n->dirtyUp(na);
    }
}

inline
UnstopAllCursor::UnstopAllCursor(VisualNode* root,
                                 const NodeAllocator& na)
    : NodeCursor<VisualNode>(root,na) {}

inline void
UnstopAllCursor::processCurrentNode(void) {
    VisualNode* n = node();
    if (n->getStatus() == STOP) {
        n->setStop(false);
        n->dirtyUp(na);
    }
}

inline
NextSolCursor::NextSolCursor(VisualNode* theNode, bool backwards,
                             const NodeAllocator& na)
    : NodeCursor<VisualNode>(theNode,na), back(backwards) {}

inline void
NextSolCursor::processCurrentNode(void) {}

inline bool
NextSolCursor::notOnSol(void) {
    return node() == startNode() || node()->getStatus() != SOLVED;
}

inline bool
NextSolCursor::mayMoveUpwards(void) {
    return notOnSol() && !node()->isRoot();
}

inline bool
NextSolCursor::mayMoveDownwards(void) {
    return notOnSol() && !(back && node() == startNode())
            && node()->hasSolvedChildren()
            && NodeCursor<VisualNode>::mayMoveDownwards();
}

inline void
NextSolCursor::moveDownwards(void) {
    NodeCursor<VisualNode>::moveDownwards();
    if (back) {
        while (NodeCursor<VisualNode>::mayMoveSidewards())
            NodeCursor<VisualNode>::moveSidewards();
    }
}

inline bool
NextSolCursor::mayMoveSidewards(void) {
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
        NodeCursor<VisualNode>::moveSidewards();
    }
}






inline
NextLeafCursor::NextLeafCursor(VisualNode* theNode, bool backwards,
                             const NodeAllocator& na)
    : NodeCursor<VisualNode>(theNode,na), back(backwards) {}

inline void
NextLeafCursor::processCurrentNode(void) {}

inline bool
NextLeafCursor::notOnLeaf(void) {
  if (node() == startNode()) return true;
  NodeStatus s = node()->getStatus();
  bool isLeaf = s == SOLVED || s == FAILED;
  return !isLeaf;
}

inline bool
NextLeafCursor::mayMoveUpwards(void) {
    return notOnLeaf() && !node()->isRoot();
}

inline bool
NextLeafCursor::mayMoveDownwards(void) {
    return notOnLeaf() && !(back && node() == startNode())
            && NodeCursor<VisualNode>::mayMoveDownwards();
}

inline void
NextLeafCursor::moveDownwards(void) {
    NodeCursor<VisualNode>::moveDownwards();
    if (back) {
        while (NodeCursor<VisualNode>::mayMoveSidewards())
            NodeCursor<VisualNode>::moveSidewards();
    }
}

inline bool
NextLeafCursor::mayMoveSidewards(void) {
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
        NodeCursor<VisualNode>::moveSidewards();
    }
}


/// **************

inline
NextPentagonCursor::NextPentagonCursor(VisualNode* theNode, bool backwards,
                             const NodeAllocator& na)
    : NodeCursor<VisualNode>(theNode,na), back(backwards) {}

inline void
NextPentagonCursor::processCurrentNode(void) {}

inline bool
NextPentagonCursor::notOnPentagon(void) {
    return node() == startNode() || node()->getStatus() != MERGING;
}

inline bool
NextPentagonCursor::mayMoveUpwards(void) {
    return notOnPentagon() && !node()->isRoot();
}

inline bool
NextPentagonCursor::mayMoveDownwards(void) {
    return notOnPentagon() && !(back && node() == startNode())
            && NodeCursor<VisualNode>::mayMoveDownwards();
}

inline void
NextPentagonCursor::moveDownwards(void) {
    NodeCursor<VisualNode>::moveDownwards();
    if (back) {
        while (NodeCursor<VisualNode>::mayMoveSidewards())
            NodeCursor<VisualNode>::moveSidewards();
    }
}

inline bool
NextPentagonCursor::mayMoveSidewards(void) {
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
        NodeCursor<VisualNode>::moveSidewards();
    }
}



/// **************

inline
StatCursor::StatCursor(VisualNode* root,
                       const NodeAllocator& na)
    : NodeCursor<VisualNode>(root,na),
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
    NodeCursor<VisualNode>::moveDownwards();
}

inline void
StatCursor::moveUpwards(void) {
    curDepth--;
    NodeCursor<VisualNode>::moveUpwards();
}

inline
SimilarShapesCursor::SimilarShapesCursor(VisualNode* root,
  const NodeAllocator& na, cpprofiler::analysis::SimilarShapesWindow& ssw)
: NodeCursor<VisualNode>(root, na), m_ssWindow(ssw) {}

//TODO(maxim): get rid of this
#include "cpprofiler/analysis/similar_shapes.hh"

inline void
SimilarShapesCursor::processCurrentNode(void) {
  VisualNode* n = node();
  int nSol = 0;
  switch (n->getStatus()) {
      case SOLVED:
        nSol = 1;
      break;
      case FAILED:
        nSol = 0;
      break;
      case SKIPPED:
        nSol = 0;
      case UNDETERMINED:
        nSol = 0;
      break;
      case BRANCH:
        nSol = 0;
        for (int i=n->getNumberOfChildren(); i--;) {
          nSol += nSols.value(n->getChild(na,i));
        }
      break;
      case STOP:
      case UNSTOP:
      case MERGING:
      break;    /// To avoid compiler warnings
  }
  nSols[n] = nSol;
  if (n->getNumberOfChildren() > 0) {
    m_ssWindow.shapeSet.insert(cpprofiler::analysis::ShapeI(nSol,n));
  }
}

inline
HighlightCursor::HighlightCursor(VisualNode* startNode,
  const NodeAllocator& na)
: NodeCursor<VisualNode>(startNode, na) {}

inline void
HighlightCursor::processCurrentNode(void) {
  VisualNode* n = node();
  n->setHighlighted(true);
}

inline
UnhighlightCursor::UnhighlightCursor(VisualNode* root,
  const NodeAllocator& na)
: NodeCursor<VisualNode>(root, na) {}

inline void
UnhighlightCursor::processCurrentNode(void) {
  VisualNode* n = node();
  n->setHighlighted(false);
}

inline
CountSolvedCursor::CountSolvedCursor(VisualNode* startNode,
  const NodeAllocator& na, int &count)
: NodeCursor<VisualNode>(startNode, na), _count(count){
  _count = 0;
}

inline void
CountSolvedCursor::processCurrentNode(void) {
  VisualNode* n = node();
  if (n->getStatus() == SOLVED){
    _count++;
  }
}

inline
GetIndexesCursor::GetIndexesCursor(VisualNode* startNode,
  const NodeAllocator& na, std::vector<int>& node_gids)
: NodeCursor<VisualNode>(startNode, na), _na(na), _node_gids(node_gids){

}

inline void
GetIndexesCursor::processCurrentNode(void) {
  VisualNode* n = node();
  _node_gids.push_back(n->getIndex(_na));
}

inline
HideNotHighlightedCursor::HideNotHighlightedCursor(VisualNode* startNode,
  const NodeAllocator& na)
  : NodeCursor<VisualNode>(startNode,na) {}

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
HideNotHighlightedCursor::mayMoveDownwards(void) {
  return NodeCursor<VisualNode>::mayMoveDownwards() &&
    (!node()->isHighlighted()) &&
    (!node()->isHidden());
}


inline
BranchLabelCursor::BranchLabelCursor(VisualNode* root, bool clear,
    NodeAllocator& na, TreeCanvas& tc)
    : NodeCursor<VisualNode>(root,na), _na(na), _tc(tc), _clear(clear) {}

inline void
BranchLabelCursor::processCurrentNode(void) {
    VisualNode* n = node();
    if (!_clear) {
        if (!na.hasLabel(n)) {
            VisualNode* p = n->getParent(_na);
            if (p) {
                int gid = n->getIndex(_na);
                std::string l = _tc.getLabel(gid);
                _na.setLabel(n,QString(l.c_str()));
//                if (n->getNumberOfChildren() < 1 &&
//                        alternative() == p->getNumberOfChildren()-1)
//                    p->purge(_na);
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
    : NodeCursor<VisualNode>(root,na) {}

inline void
DisposeCursor::processCurrentNode(void) {
    node()->dispose();
}

inline
SubtreeCountCursor::SubtreeCountCursor(VisualNode *theNode,
                   int _threshold,
                   const NodeAllocator& na)
  : NodeCursor<VisualNode>(theNode, na),
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
    NodeCursor<VisualNode>::moveSidewards();
    int x = stack.back();
    stack.pop_back();
    stack.back() += x;
    stack.push_back(0);
}

inline void
SubtreeCountCursor::moveUpwards(void) {
    NodeCursor<VisualNode>::moveUpwards();
    int x = stack.back();
    stack.pop_back();
    stack.back() += x;
}

inline void
SubtreeCountCursor::moveDownwards(void) {
    NodeCursor<VisualNode>::moveDownwards();
    stack.push_back(0);
}



inline
SearchLogCursor::SearchLogCursor(VisualNode *theNode,
                                 QTextStream& outputStream,
                                 const NodeAllocator& na,
                                 const Execution& execution)
    : NodeCursor<VisualNode>(theNode, na),
      _out(outputStream),
      _na(na),
      _execution(execution)
{}

inline void
SearchLogCursor::processCurrentNode(void) {
    VisualNode* n = node();
    int numChildren = n->getNumberOfChildren();

    // This node's index and the number of children.
    _out << n->getIndex(_na) << " " << numChildren;

    for (int i = 0 ; i < numChildren ; i++) {
        int childIndex = n->getChild(i);

        int child_gid = n->getChild(i);
        auto child_label = QString::fromStdString(_execution.getLabel(child_gid));

        // The child's index and its label.
        _out << " " << childIndex << " " << child_label;
    }

    // Terminate the line.
    _out << "\n";
}

inline
UnhideAncestorsCursor::UnhideAncestorsCursor(VisualNode* root,
                                 const NodeAllocator& na)
    : NodeCursor<VisualNode>(root,na) {}

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
UnhideAncestorsCursor::mayMoveUpwards(void) {
    return !node()->isRoot();
}

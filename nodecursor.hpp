
template<class Node>
inline
NodeCursor<Node>::NodeCursor(Node* theNode,
                             const typename Node::NodeAllocator& na0)
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
NodeCursor<Node>::mayMoveDownwards(void) { /// bookmark
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
    VisualNode* n = node();
    return (!onlyDirty || n->isDirty()) &&
            NodeCursor<VisualNode>::mayMoveDownwards() &&
            (n->hasSolvedChildren() || n->getNoOfOpenChildren(na) > 0) &&
            (! n->isHidden());
}

inline
HideFailedCursor::HideFailedCursor(VisualNode* root,
                                   const VisualNode::NodeAllocator& na,
                                   bool onlyDirtyNodes)
    : NodeCursor<VisualNode>(root,na), onlyDirty(onlyDirtyNodes) {}

inline void
HideFailedCursor::processCurrentNode(void) {
    VisualNode* n = node();
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
                                 const VisualNode::NodeAllocator& na)
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
UnstopAllCursor::UnstopAllCursor(VisualNode* root,
                                 const VisualNode::NodeAllocator& na)
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
                             const VisualNode::NodeAllocator& na)
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
StatCursor::StatCursor(VisualNode* root,
                       const VisualNode::NodeAllocator& na)
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
AnalyzeCursor::AnalyzeCursor(VisualNode* root, 
  const VisualNode::NodeAllocator& na, TreeCanvas* tc)
: NodeCursor<VisualNode>(root, na), _tc(tc) {}

inline void
AnalyzeCursor::processCurrentNode(void) {
  VisualNode* n = node();
  int nSol;
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
  if (n->getNumberOfChildren() > 0)
    _tc->shapesMap.insert(ShapeI(nSol,n));
}

inline
HighlightCursor::HighlightCursor(VisualNode* startNode, 
  const VisualNode::NodeAllocator& na)
: NodeCursor<VisualNode>(startNode, na) {}

inline void
HighlightCursor::processCurrentNode(void) {
  VisualNode* n = node();
  n->setHighlighted(true);
}

inline
UnhighlightCursor::UnhighlightCursor(VisualNode* root, 
  const VisualNode::NodeAllocator& na)
: NodeCursor<VisualNode>(root, na) {}

inline void
UnhighlightCursor::processCurrentNode(void) {
  VisualNode* n = node();
  n->setHighlighted(false);
}

inline
CountSolvedCursor::CountSolvedCursor(VisualNode* startNode, 
  const VisualNode::NodeAllocator& na, int &count)
: NodeCursor<VisualNode>(startNode, na), _count(count){
  _count = 0;
}

inline void
CountSolvedCursor::processCurrentNode(void) {
  VisualNode* n = node();
  if (n->getStatus() == SOLVED){
    _count++;
  }
  // n->setHighlighted(false);
  // if leaf and solved count
}

inline
HideNotHighlightedCursor::HideNotHighlightedCursor(VisualNode* startNode,
  const VisualNode::NodeAllocator& na)
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
                                     VisualNode::NodeAllocator& na)
    : NodeCursor<VisualNode>(root,na), _na(na), _clear(clear) {}

inline void
BranchLabelCursor::processCurrentNode(void) {
    VisualNode* n = node();
    if (!_clear) {
        if (!na.hasLabel(n)) {
            VisualNode* p = n->getParent(_na);
            if (p) {
                int gid = n->getIndex(_na);
                std::string l = Data::current->getLabelByGid(gid);
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
                             const VisualNode::NodeAllocator& na)
    : NodeCursor<VisualNode>(root,na) {}

inline void
DisposeCursor::processCurrentNode(void) {
    node()->dispose();
}


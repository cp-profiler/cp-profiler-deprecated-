#include "spacenode.hh"
#include "visualnode.hh"

#include <QString>
#include <QVector>

  void
  SpaceNode::closeChild(const NodeAllocator& na,
                        bool hadFailures, bool hadSolutions) {
    setHasFailedChildren(hasFailedChildren() || hadFailures);
    setHasSolvedChildren(hasSolvedChildren() || hadSolutions);

    bool allClosed = true;
    for (int i=getNumberOfChildren(); i--;) {
      if (getChild(na,i)->isOpen()) {
        allClosed = false;
        break;
      }
    }

    if (allClosed) {
      setHasOpenChildren(false);
      for (unsigned int i=0; i<getNumberOfChildren(); i++)
        setHasSolvedChildren(hasSolvedChildren() ||
          getChild(na,i)->hasSolvedChildren());
      SpaceNode* p = getParent(na);
      if (p != NULL) {
        p->closeChild(na, hasFailedChildren(), hasSolvedChildren());
      }
    } else {
      
      if (hadSolutions) {
        setHasSolvedChildren(true);
        SpaceNode* p = getParent(na);
        while (p != NULL && !p->hasSolvedChildren()) {
          p->setHasSolvedChildren(true);
          p = p->getParent(na);
        }
      }
      if (hadFailures) {
        SpaceNode* p = getParent(na);
        while (p != NULL && !p->hasFailedChildren()) {
          p->setHasFailedChildren(true);
          p = p->getParent(na);
        }        
      }
    }

  }

  SpaceNode::SpaceNode(int db_id0,bool)
  : Node(-1, false), db_id(db_id0),
    nstatus(0) {
//    if (root == NULL) {
//      setStatus(FAILED);
//      setHasSolvedChildren(false);
//      setHasFailedChildren(true);
//    } else {
      setStatus(UNDETERMINED);
      setHasSolvedChildren(false);
      setHasFailedChildren(false);
//    }
  }


  int
  SpaceNode::getNumberOfChildNodes(NodeAllocator& na) {
    int kids = 0;
    if (isUndetermined()) {
        SpaceNode* p = getParent(na);
        if (p==NULL) {
            kids = 2;
            setStatus(BRANCH);
            setHasOpenChildren(true);
        } else {
            kids = 0;
            setStatus(SOLVED);
            setHasOpenChildren(false);
            setHasSolvedChildren(true);
            setHasFailedChildren(false);
        }
        if (p != NULL)
            p->closeChild(na, false, true);
        setNumberOfChildren(kids, na);
    }
//      stats.undetermined--;
//      acquireSpace(na, curBest, c_d, a_d);
//      QVector<QString> labels;
//      switch (static_cast<Space*>(Support::funmark(copy))->status(stats)) {
//      case SS_FAILED:
//        {
//          purge(na);
//          kids = 0;
//          setHasOpenChildren(false);
//          setHasSolvedChildren(false);
//          setHasFailedChildren(true);
//          setStatus(FAILED);
//          stats.failures++;
//          SpaceNode* p = getParent(na);
//          if (p != NULL)
//            p->closeChild(na, true, false);
//        }
//        break;
//      case SS_SOLVED:
//        {
//          // Deletes all pending branchers
//          (void) static_cast<Space*>(Support::funmark(copy))->choice();
//          kids = 0;
//          setStatus(SOLVED);
//          setHasOpenChildren(false);
//          setHasSolvedChildren(true);
//          setHasFailedChildren(false);
//          stats.solutions++;
//          if (curBest != NULL) {
//            curBest->s = this;
//          }
//          SpaceNode* p = getParent(na);
//          if (p != NULL)
//            p->closeChild(na, false, true);
//        }
//        break;
//      case SS_BRANCH:
//        {
//          Space* s = static_cast<Space*>(Support::funmark(copy));
//          choice = s->choice();
//          kids = choice->alternatives();
//          setHasOpenChildren(true);
//          if (dynamic_cast<const StopChoice*>(choice)) {
//            setStatus(STOP);
//          } else {
//            setStatus(BRANCH);
//            stats.choices++;
//          }
//          stats.undetermined += kids;
//          for (int i=0; i<kids; i++) {
//            std::ostringstream oss;
//            s->print(*choice,i,oss);
//            labels.push_back(QString(oss.str().c_str()));
//          }
//        }
//        break;
//      }
//      static_cast<VisualNode*>(this)->changedStatus(na);
//      setNumberOfChildren(kids, na);
//    } else {
//      kids = getNumberOfChildren();
//    }
    return kids;
  }

  int
  SpaceNode::getNoOfOpenChildren(const NodeAllocator& na) {
    if (!hasOpenChildren())
      return 0;
    int noOfOpenChildren = 0;
    for (int i=getNumberOfChildren(); i--;)
      noOfOpenChildren += (getChild(na,i)->isOpen());
    return noOfOpenChildren;
  }

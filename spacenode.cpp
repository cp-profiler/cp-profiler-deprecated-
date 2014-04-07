#include "spacenode.hh"
#include "visualnode.hh"

#include "data.hh"

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
  : Node(-1, false), db_id(db_id0),  /// ???
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
    // int kids = noOfChildren;
    int kids = Data::self->getKids(db_id);

    // if (isUndetermined()) {
    //   if (kids > 0) {
    //     // setStatus(BRANCH);
    //     // setHasOpenChildren(true);
    //   } else {
    //     // setStatus(SOLVED);
    //     // setHasOpenChildren(false);
    //     // setHasSolvedChildren(true);
    //     // setHasFailedChildren(false);
    //   }
    if (db_id >= 0)
      setNumberOfChildren(kids, na);

    // }
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

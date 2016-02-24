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
      if (p != nullptr) {
        p->closeChild(na, hasFailedChildren(), hasSolvedChildren());
      }
    } else {

      if (hadSolutions) {
        setHasSolvedChildren(true);
        SpaceNode* p = getParent(na);
        while (p != nullptr && !p->hasSolvedChildren()) {
          p->setHasSolvedChildren(true);
          p = p->getParent(na);
        }
      }
      if (hadFailures) {
        SpaceNode* p = getParent(na);
        while (p != nullptr && !p->hasFailedChildren()) {
          p->setHasFailedChildren(true);
          p = p->getParent(na);
        }
      }
    }

  }

  SpaceNode::SpaceNode(bool)
  : Node(-1, false),
    nstatus(0) {
//    if (root == nullptr) {
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
  SpaceNode::getNoOfOpenChildren(const NodeAllocator& na) {
    if (!hasOpenChildren())
      return 0;
    int noOfOpenChildren = 0;
    for (int i=getNumberOfChildren(); i--;)
      noOfOpenChildren += (getChild(na,i)->isOpen());
    return noOfOpenChildren;
  }


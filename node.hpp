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

#ifndef NODE_HPP
#define NODE_HPP

inline unsigned int
Node::getTag(void) const {
  return static_cast<unsigned int>
    (reinterpret_cast<ptrdiff_t>(childrenOrFirstChild) & 3);
}

inline void
Node::setTag(unsigned int tag) {
  assert(tag <= 3);
  assert(getTag() == UNDET);
  childrenOrFirstChild = reinterpret_cast<void*>
    ( (reinterpret_cast<ptrdiff_t>(childrenOrFirstChild) & ~(3)) | tag);
}

inline void*
Node::getPtr(void) const {
  return reinterpret_cast<void*>
    (reinterpret_cast<ptrdiff_t>(childrenOrFirstChild) & ~(3));
}

inline int
Node::getFirstChild(void) const {
  return static_cast<int>
    ((reinterpret_cast<ptrdiff_t>(childrenOrFirstChild) & ~(3)) >> 2);
}

inline
Node::Node(int p, bool failed) : parent(p) {
  childrenOrFirstChild = nullptr;
  noOfChildren = 0;
  setTag(failed ? LEAF : UNDET);

#ifdef MAXIM_DEBUG
  debug_id = Node::debug_instance_counter++;
#endif
}

inline int
Node::getParent(void) const {
  return parent;
}

inline VisualNode*
Node::getParent(const NodeAllocator& na) const {
  return parent < 0 ? nullptr : na[parent];
}

inline bool
Node::isUndetermined(void) const { return getTag() == UNDET; }

inline int
Node::getChild(int n) const {
  assert(getTag() != UNDET && getTag() != LEAF);
  if (getTag() == TWO_CHILDREN) {
    assert(n != 1 || noOfChildren <= 0);
    return n == 0 ? getFirstChild() : -noOfChildren;
  }
  assert(n < noOfChildren);
  return static_cast<int*>(getPtr())[n];
}

inline VisualNode*
Node::getChild(const NodeAllocator& na, int n) const {
  return na[getChild(n)];
}

inline bool
Node::isRoot(void) const { return parent == -1; }

inline unsigned int
Node::getNumberOfChildren(void) const {
  switch (getTag()) {
  case UNDET: return 0;
  case LEAF:  return 0;
  case TWO_CHILDREN: return 1+(noOfChildren <= 0);
  default: return noOfChildren;
  }
}

inline int
Node::getIndex(const NodeAllocator& na) const {
//  int j;
  if (parent==-1)
    return 0;
  Node* p = na[parent];
  for (int i=p->getNumberOfChildren(); i--;)
    if (p->getChild(na,i) == this){
//      j = p->getChild(i);
      return p->getChild(i);
    }
  GECODE_NEVER;
   return -1;
}


#endif // NODE_HPP

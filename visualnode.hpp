/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Guido Tack, 2007
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

#ifndef VISUALNODE_HPP
#define VISUALNODE_HPP

#include <iostream>

#ifdef MAXIM_DEBUG
#include <QDebug>
#endif

inline NodeAllocator::NodeAllocator() {
  nodes.reserve(1000);
}

inline NodeAllocator::~NodeAllocator(void) {
  for (auto &node : nodes) {
    delete node;
  }
}

inline int NodeAllocator::allocate(int p) {
  nodes.push_back(new VisualNode{p});
  return nodes.size() - 1;
}

inline int NodeAllocator::allocateRoot() {
#ifdef MAXIM_DEBUG
  qDebug() << "allocated root";
#endif
  nodes.push_back(new VisualNode{});
  return nodes.size() - 1;
}

inline VisualNode* NodeAllocator::operator[](int i) const {
#ifdef MAXIM_DEBUG
  // qDebug() << "nodes[" << i << "]";
#endif
  assert(static_cast<uint>(i) < nodes.size());
  return nodes[i];
}

inline bool NodeAllocator::hasLabel(VisualNode* n) const {
  return labels.contains(n);
}

inline void NodeAllocator::setLabel(VisualNode* n, const QString& l) {
  labels[n] = l;
}

inline void NodeAllocator::clearLabel(VisualNode* n) { labels.remove(n); }

inline QString NodeAllocator::getLabel(VisualNode* n) const {
#ifdef MAXIM_DEBUG
   // return " " + QString::number(n->debug_id) + " ";
#endif
  return labels.value(n);
}

inline int NodeAllocator::size() const {
  return nodes.size();
}

inline Extent::Extent(void) : l(-1), r(-1) {}

inline Extent::Extent(int l0, int r0) : l(l0), r(r0) {}

inline Extent::Extent(int width) {
  int halfWidth = width / 2;
  l = 0 - halfWidth;
  r = 0 + halfWidth;
}

inline void Extent::extend(int deltaL, int deltaR) {
  l += deltaL;
  r += deltaR;
}

inline void Extent::move(int delta) {
  l += delta;
  r += delta;
}

inline int Shape::depth(void) const { return _depth; }

inline void Shape::setDepth(int d) {
  assert(d <= _depth);
  _depth = d;
}

inline const Extent& Shape::operator[](int i) const {
  assert(i < _depth);
  return shape[i];
}

inline Extent& Shape::operator[](int i) {
  assert(i < _depth);
  return shape[i];
}

inline Shape* Shape::allocate(int d) {
  assert(d >= 1);
  Shape* ret;
  ret = static_cast<Shape*>(
      heap.ralloc(sizeof(Shape) + (d - 1) * sizeof(Extent)));
  ret->_depth = d;
  return ret;
}

inline void Shape::deallocate(Shape* shape) {
  if (shape != hidden && shape != leaf) heap.rfree(shape);
}

inline bool Shape::getExtentAtDepth(int d, Extent& extent) {
  if (d > depth()) return false;
  extent = Extent(0, 0);
  for (int i = 0; i <= d; i++) {
    Extent currentExtent = (*this)[i];
    extent.l += currentExtent.l;
    extent.r += currentExtent.r;
  }
  return true;
}

inline void Shape::computeBoundingBox(void) {
  int lastLeft = 0;
  int lastRight = 0;
  bb.left = 0;
  bb.right = 0;
  for (int i = 0; i < depth(); i++) {
    lastLeft = lastLeft + (*this)[i].l;
    lastRight = lastRight + (*this)[i].r;
    bb.left = std::min(bb.left, lastLeft);
    bb.right = std::max(bb.right, lastRight);
  }
}

inline const BoundingBox& Shape::getBoundingBox(void) const { return bb; }

inline bool VisualNode::isHidden(void) const { return getFlag(HIDDEN); }

inline void VisualNode::setHidden(bool h) { setFlag(HIDDEN, h); }

inline int VisualNode::getOffset(void) { return offset; }

inline void VisualNode::setOffset(int n) { offset = n; }

inline bool VisualNode::isDirty(void) const { return getFlag(DIRTY); }

inline void VisualNode::setDirty(bool d) {
  // std::cerr << "node [" << this->debug_id << "] set dirty: " << d << "\n"; 
  setFlag(DIRTY, d);
}

inline bool VisualNode::childrenLayoutIsDone(void) {
  return getFlag(CHILDRENLAYOUTDONE);
}

inline void VisualNode::setChildrenLayoutDone(bool d) {
  setFlag(CHILDRENLAYOUTDONE, d);
}

inline bool VisualNode::isMarked(void) const { return getFlag(MARKED); }

inline void VisualNode::setMarked(bool m) { setFlag(MARKED, m); }

inline bool VisualNode::isSelected(void) const { return getFlag(SELECTED); }

inline void VisualNode::setSelected(bool m) { setFlag(SELECTED, m); }

inline bool VisualNode::isHovered(void) const { return getFlag(HOVEREDOVER); }

inline void VisualNode::setHovered(bool m) { setFlag(HOVEREDOVER, m); }

inline bool VisualNode::isBookmarked(void) const { return getFlag(BOOKMARKED); }

inline void VisualNode::setBookmarked(bool m) { setFlag(BOOKMARKED, m); }

inline bool VisualNode::isHighlighted(void) const { return getFlag(HIGHLIGHTED); }

inline void VisualNode::setHighlighted(bool m) { setFlag(HIGHLIGHTED, m); }

inline bool VisualNode::isOnPath(void) const { return getFlag(ONPATH); }

inline void VisualNode::setOnPath(bool b) { setFlag(ONPATH, b); }

inline void VisualNode::setSubtreeSizeUnknown(void) {
  setNumericFlag(SUBTREESIZE, 3, 7);
}

inline void VisualNode::setSubtreeSize(int size) {
  setNumericFlag(SUBTREESIZE, 3, size);
}

inline int VisualNode::getSubtreeSize(void) {
  int size = getNumericFlag(SUBTREESIZE, 3);
  if (size == 7) return -1;
  return size;
}

inline Shape* VisualNode::getShape(void) const {
  if (isHidden()) return (getStatus() == MERGING) ? Shape::leaf : Shape::hidden;
  return shape;
}

inline BoundingBox VisualNode::getBoundingBox(void) const {
  return getShape()->getBoundingBox();
}

#endif  // VISUALNODE_HPP

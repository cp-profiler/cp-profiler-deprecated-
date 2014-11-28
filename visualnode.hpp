#ifndef VISUALNODE_HPP
#define VISUALNODE_HPP

inline
Extent::Extent(void) : l(-1), r(-1) {}

inline
Extent::Extent(int l0, int r0) : l(l0), r(r0) {}

inline
Extent::Extent(int width) {
  int halfWidth = width / 2;
  l = 0 - halfWidth;
  r = 0 + halfWidth;
}

inline void
Extent::extend(int deltaL, int deltaR) {
  l += deltaL; r += deltaR;
}

inline void
Extent::move(int delta) {
  l += delta; r += delta;
}

inline int
Shape::depth(void) const { return _depth; }

inline void
Shape::setDepth(int d) {
  assert(d <= _depth);
  _depth = d;
}

inline const Extent&
Shape::operator [](int i) const {
  assert(i < _depth);
  return shape[i];
}

inline Extent&
Shape::operator [](int i) {
  assert(i < _depth);
  return shape[i];
}

inline Shape*
Shape::allocate(int d) {
  assert(d >= 1);
  Shape* ret;
  ret =
    static_cast<Shape*>(heap.ralloc(sizeof(Shape)+(d-1)*sizeof(Extent)));
  ret->_depth = d;
  return ret;
}

inline void
Shape::deallocate(Shape* shape) {
  if (shape != hidden && shape != leaf)
    heap.rfree(shape);
}

inline bool
Shape::getExtentAtDepth(int d, Extent& extent) {
  if (d > depth())
    return false;
  extent = Extent(0,0);
  for (int i=0; i <= d; i++) {
    Extent currentExtent = (*this)[i];
    extent.l += currentExtent.l;
    extent.r += currentExtent.r;
  }
  return true;
}

inline void
Shape::computeBoundingBox(void) {
  int lastLeft = 0;
  int lastRight = 0;
  bb.left = 0;
  bb.right = 0;
  for (int i=0; i<depth(); i++) {
    lastLeft = lastLeft + (*this)[i].l;
    lastRight = lastRight + (*this)[i].r;
    bb.left = std::min(bb.left,lastLeft);
    bb.right = std::max(bb.right,lastRight);
  }
}

inline const BoundingBox&
Shape::getBoundingBox(void) const {
  return bb;
}

inline bool
VisualNode::isHidden(void) {
  return getFlag(HIDDEN);
}

inline void
VisualNode::setHidden(bool h) {
  setFlag(HIDDEN, h);
}

inline void
VisualNode::setStop(bool h) {
  if (getStatus() == BRANCH && h)
    setStatus(STOP);
  else if (getStatus() == STOP && !h)
    setStatus(UNSTOP);
}

inline int
VisualNode::getOffset(void) { return offset; }

inline void
VisualNode::setOffset(int n) { offset = n; }

inline bool
VisualNode::isDirty(void) {
  return getFlag(DIRTY);
}

inline void
VisualNode::setDirty(bool d) {
  setFlag(DIRTY, d);
}

inline bool
VisualNode::childrenLayoutIsDone(void) {
  return getFlag(CHILDRENLAYOUTDONE);
}

inline void
VisualNode::setChildrenLayoutDone(bool d) {
  setFlag(CHILDRENLAYOUTDONE, d);
}

inline bool
VisualNode::isMarked(void) {
  return getFlag(MARKED);
}

inline void
VisualNode::setMarked(bool m) {
  setFlag(MARKED, m);
}

inline bool
VisualNode::isBookmarked(void) {
  return getFlag(BOOKMARKED);
}

inline void
VisualNode::setBookmarked(bool m) {
  setFlag(BOOKMARKED, m);
}

inline bool
VisualNode::isHighlighted(void) {
  return getFlag(HIGHLIGHTED);
}

inline void
VisualNode::setHighlighted(bool m) {
  setFlag(HIGHLIGHTED, m);
}

inline bool
VisualNode::isOnPath(void) {
  return getFlag(ONPATH);
}

inline void
VisualNode::setOnPath(bool b) {
  setFlag(ONPATH, b);
}

inline Shape*
VisualNode::getShape(void) {
  if (isHidden())
    return (getStatus() == MERGING) ? Shape::leaf : Shape::hidden;
  return shape;
}

inline BoundingBox
VisualNode::getBoundingBox(void) { return getShape()->getBoundingBox(); }

#endif // VISUALNODE_HPP

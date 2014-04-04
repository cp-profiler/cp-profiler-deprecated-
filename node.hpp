#ifndef NODE_HPP
#define NODE_HPP

#include "data.hh"

template<class T>
void
NodeAllocatorBase<T>::allocate(void) {
  cur_b++;
  cur_t = 0;
  if (cur_b==n) {
    int oldn = n;
    n = static_cast<int>(n*1.5+1.0);
    b = heap.realloc<Block*>(b,oldn,n);
  }
  b[cur_b] = static_cast<Block*>(heap.ralloc(sizeof(Block)));
}

template<class T>
NodeAllocatorBase<T>::NodeAllocatorBase(bool bab)
  : _bab(bab) {
  b = heap.alloc<Block*>(10);
  n = 10;
  cur_b = -1;
  cur_t = NodeBlockSize-1;
}

template<class T>
NodeAllocatorBase<T>::~NodeAllocatorBase(void) {
  for (int i=cur_b+1; i--;)
    heap.rfree(b[i]);
  heap.free<Block*>(b,n);
}

template<class T>
inline int
NodeAllocatorBase<T>::allocate(int p, int _db_id) {
  cur_t++;
  if (cur_t==NodeBlockSize)
    allocate();
  // new (&b[cur_b]->b[cur_t]) T(db_id, false); /// bookmark
  new (&b[cur_b]->b[cur_t]) T(p, _db_id); /// bookmark
  b[cur_b]->best[cur_t] = -1;
  return cur_b*NodeBlockSize+cur_t;
}

template<class T>
inline int
NodeAllocatorBase<T>::allocateRoot(int db_id) {
  cur_t++;
  if (cur_t==NodeBlockSize)
    allocate();
  new (&b[cur_b]->b[cur_t]) T(db_id,true);
  b[cur_b]->best[cur_t] = -1;
  return cur_b*NodeBlockSize+cur_t;
}

template<class T>
inline T*
NodeAllocatorBase<T>::operator [](int i) const {
  assert(i/NodeBlockSize < n);
  assert(i/NodeBlockSize < cur_b || i%NodeBlockSize <= cur_t);
  return &(b[i/NodeBlockSize]->b[i%NodeBlockSize]);
}

template<class T>
inline T*
NodeAllocatorBase<T>::best(int i) const {
  assert(i/NodeBlockSize < n);
  assert(i/NodeBlockSize < cur_b || i%NodeBlockSize <= cur_t);
  int bi = b[i/NodeBlockSize]->best[i%NodeBlockSize];
  return bi == -1 ? NULL : (*this)[bi];
}

template<class T>
inline void
NodeAllocatorBase<T>::setBest(int i, int best) {
  assert(i/NodeBlockSize < n);
  assert(i/NodeBlockSize < cur_b || i%NodeBlockSize <= cur_t);
  b[i/NodeBlockSize]->best[i%NodeBlockSize] = best;
}

template<class T>
inline bool
NodeAllocatorBase<T>::bab(void) const {
  return _bab;
}

template<class T>
inline bool
NodeAllocatorBase<T>::showLabels(void) const {
  return !labels.isEmpty();
}

template<class T>
bool
NodeAllocatorBase<T>::hasLabel(T* n) const {
  return labels.contains(n);
}

template<class T>
void
NodeAllocatorBase<T>::setLabel(T* n, const QString& l) {
  labels[n] = l;
}

template<class T>
void
NodeAllocatorBase<T>::clearLabel(T* n) {
  labels.remove(n);
}

template<class T>
QString
NodeAllocatorBase<T>::getLabel(T* n) const {
  return labels.value(n);
}

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
  childrenOrFirstChild = NULL;
  noOfChildren = 0;
  setTag(failed ? LEAF : UNDET);
}

inline int
Node::getParent(void) const {
  return parent;
}

inline VisualNode*
Node::getParent(const NodeAllocator& na) const {
  return parent < 0 ? NULL : na[parent];
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
  int j;
  if (parent==-1)
    return 0;
  Node* p = na[parent];
  for (int i=p->getNumberOfChildren(); i--;)
    if (p->getChild(na,i) == this){
      j = p->getChild(i);
      return p->getChild(i);
    }
  GECODE_NEVER;
   return -1;
}

inline int
Node::getNewIndex(void) const {
  return Data::self->getIndex();
}



#endif // NODE_HPP

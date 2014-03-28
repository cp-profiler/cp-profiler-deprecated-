#ifndef HEAP_HPP
#define HEAP_HPP

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <algorithm>

/**
 * \brief %Heap memory management class
 *
 * All routines throw an exception of MemoryExhausted, if a request
 * cannot be fulfilled.
 * \ingroup FuncMemHeap
 */
class Heap {
public:
  /// Default constructor (ensuring that only a single instance is created)
  Heap(void);
  /// \name Typed allocation routines
  //@{
  /**
   * \brief Allocate block of \a n objects of type \a T from heap
   *
   * Note that this function implements C++ semantics: the default
   * constructor of \a T is run for all \a n objects.
   */
  template<class T>
  T* alloc(long unsigned int n);
  /**
   * \brief Allocate block of \a n objects of type \a T from heap
   *
   * Note that this function implements C++ semantics: the default
   * constructor of \a T is run for all \a n objects.
   */
  template<class T>
  T* alloc(long int n);
  /**
   * \brief Allocate block of \a n objects of type \a T from heap
   *
   * Note that this function implements C++ semantics: the default
   * constructor of \a T is run for all \a n objects.
   */
  template<class T>
  T* alloc(unsigned int n);
  /**
   * \brief Allocate block of \a n objects of type \a T from heap
   *
   * Note that this function implements C++ semantics: the default
   * constructor of \a T is run for all \a n objects.
   */
  template<class T>
  T* alloc(int n);
  /**
   * \brief Delete \a n objects starting at \a b
   *
   * Note that this function implements C++ semantics: the destructor
   * of \a T is run for all \a n objects.
   */
  template<class T>
  void free(T* b, long unsigned int n);
  /**
   * \brief Delete \a n objects starting at \a b
   *
   * Note that this function implements C++ semantics: the destructor
   * of \a T is run for all \a n objects.
   */
  template<class T>
  void free(T* b, long int n);
  /**
   * \brief Delete \a n objects starting at \a b
   *
   * Note that this function implements C++ semantics: the destructor
   * of \a T is run for all \a n objects.
   */
  template<class T>
  void free(T* b, unsigned int n);
  /**
   * \brief Delete \a n objects starting at \a b
   *
   * Note that this function implements C++ semantics: the destructor
   * of \a T is run for all \a n objects.
   */
  template<class T>
  void free(T* b, int n);
  /**
   * \brief Reallocate block of \a n objects starting at \a b to \a m objects of type \a T from heap
   *
   * Note that this function implements C++ semantics: the copy constructor
   * of \a T is run for all \f$\min(n,m)\f$ objects, the default
   * constructor of \a T is run for all remaining
   * \f$\max(n,m)-\min(n,m)\f$ objects, and the destrucor of \a T is
   * run for all \a n objects in \a b.
   *
   * Returns the address of the new block.
   */
  template<class T>
  T* realloc(T* b, long unsigned int n, long unsigned int m);
  /**
   * \brief Reallocate block of \a n objects starting at \a b to \a m objects of type \a T from heap
   *
   * Note that this function implements C++ semantics: the copy constructor
   * of \a T is run for all \f$\min(n,m)\f$ objects, the default
   * constructor of \a T is run for all remaining
   * \f$\max(n,m)-\min(n,m)\f$ objects, and the destrucor of \a T is
   * run for all \a n objects in \a b.
   *
   * Returns the address of the new block.
   */
  template<class T>
  T* realloc(T* b, long int n, long int m);
  /**
   * \brief Reallocate block of \a n objects starting at \a b to \a m objects of type \a T from heap
   *
   * Note that this function implements C++ semantics: the copy constructor
   * of \a T is run for all \f$\min(n,m)\f$ objects, the default
   * constructor of \a T is run for all remaining
   * \f$\max(n,m)-\min(n,m)\f$ objects, and the destrucor of \a T is
   * run for all \a n objects in \a b.
   *
   * Returns the address of the new block.
   */
  template<class T>
  T* realloc(T* b, unsigned int n, unsigned int m);
  /**
   * \brief Reallocate block of \a n objects starting at \a b to \a m objects of type \a T from heap
   *
   * Note that this function implements C++ semantics: the copy constructor
   * of \a T is run for all \f$\min(n,m)\f$ objects, the default
   * constructor of \a T is run for all remaining
   * \f$\max(n,m)-\min(n,m)\f$ objects, and the destrucor of \a T is
   * run for all \a n objects in \a b.
   *
   * Returns the address of the new block.
   */
  template<class T>
  T* realloc(T* b, int n, int m);
  /**
   * \brief Reallocate block of \a n pointers starting at \a b to \a m objects of type \a T* from heap
   *
   * Returns the address of the new block.
   *
   * This is a specialization for performance.
   */
  template<class T>
  T** realloc(T** b, long unsigned int n, long unsigned int m);
  /**
   * \brief Reallocate block of \a n pointers starting at \a b to \a m objects of type \a T* from heap
   *
   * Returns the address of the new block.
   *
   * This is a specialization for performance.
   */
  template<class T>
  T** realloc(T** b, long int n, long int m);
  /**
   * \brief Reallocate block of \a n pointers starting at \a b to \a m objects of type \a T* from heap
   *
   * Returns the address of the new block.
   *
   * This is a specialization for performance.
   */
  template<class T>
  T** realloc(T** b, unsigned int n, unsigned int m);
  /**
   * \brief Reallocate block of \a n pointers starting at \a b to \a m objects of type \a T* from heap
   *
   * Returns the address of the new block.
   *
   * This is a specialization for performance.
   */
  template<class T>
  T** realloc(T** b, int n, int m);
  /**
   * \brief Copy \a n objects starting at \a s to \a d
   *
   * Note that this function implements C++ semantics: the assignment
   * operator of \a T is run for all \a n objects.
   *
   * Returns \a d.
   */
  template<class T>
  static T* copy(T* d, const T* s, long unsigned int n);
  /**
   * \brief Copy \a n objects starting at \a s to \a d
   *
   * Note that this function implements C++ semantics: the assignment
   * operator of \a T is run for all \a n objects.
   *
   * Returns \a d.
   */
  template<class T>
  static T* copy(T* d, const T* s, long int n);
  /**
   * \brief Copy \a n objects starting at \a s to \a d
   *
   * Note that this function implements C++ semantics: the assignment
   * operator of \a T is run for all \a n objects.
   *
   * Returns \a d.
   */
  template<class T>
  static T* copy(T* d, const T* s, unsigned int n);
  /**
   * \brief Copy \a n objects starting at \a s to \a d
   *
   * Note that this function implements C++ semantics: the assignment
   * operator of \a T is run for all \a n objects.
   *
   * Returns \a d.
   */
  template<class T>
  static T* copy(T* d, const T* s, int n);
  /**
   * \brief Copy \a n pointers starting at \a s to \a d
   *
   * Returns \a d.
   *
   * This is a specialization for performance.
   */
  template<class T>
  static T** copy(T** d, const T** s, long unsigned int n);
  /**
   * \brief Copy \a n pointers starting at \a s to \a d
   *
   * Returns \a d.
   *
   * This is a specialization for performance.
   */
  template<class T>
  static T** copy(T** d, const T** s, long int n);
  /**
   * \brief Copy \a n pointers starting at \a s to \a d
   *
   * Returns \a d.
   *
   * This is a specialization for performance.
   */
  template<class T>
  static T** copy(T** d, const T** s, unsigned int n);
  /**
   * \brief Copy \a n pointers starting at \a s to \a d
   *
   * Returns \a d.
   *
   * This is a specialization for performance.
   */
  template<class T>
  static T** copy(T** d, const T** s, int n);
  //@}
  /// \name Raw allocation routines
  //@{
  /// Allocate \a s bytes from heap
  void* ralloc(size_t s);
  /// Free memory block starting at \a p
  void  rfree(void* p);
  /// Free memory block starting at \a p with size \a s
  void  rfree(void* p, size_t s);
  /// Change memory block starting at \a p to size \a s
  void* rrealloc(void* p, size_t s);
  //@}
private:
  /// Allocate memory from heap (disabled)
  static void* operator new(size_t s) throw() { (void) s; return NULL; }
  /// Free memory allocated from heap (disabled)
  static void  operator delete(void* p) { (void) p; };
  /// Copy constructor (disabled)
  Heap(const Heap&) {}
  /// Assignment operator (disabled)
  const Heap& operator =(const Heap&) { return *this; }
};

/// The single global heap
extern Heap heap;

/*
 * Wrappers for raw allocation routines
 *
 */
inline void*
Heap::ralloc(size_t s) {
  void* p = ::malloc(s);
  return p;
}

inline void
Heap::rfree(void* p) {
  ::free(p);
}

inline void
Heap::rfree(void* p, size_t) {
  ::free(p);
}

inline void*
Heap::rrealloc(void* p, size_t s) {
  p = ::realloc(p,s);
  return p;
}


/*
 * Typed allocation routines
 *
 */
template<class T>
inline T*
Heap::alloc(long unsigned int n) {
  T* p = static_cast<T*>(ralloc(sizeof(T)*n));
  for (long unsigned int i=n; i--; )
    (void) new (p+i) T();
  return p;
}
template<class T>
inline T*
Heap::alloc(long int n) {
  assert(n >= 0);
  return alloc<T>(static_cast<long unsigned int>(n));
}
template<class T>
inline T*
Heap::alloc(unsigned int n) {
  return alloc<T>(static_cast<long unsigned int>(n));
}
template<class T>
inline T*
Heap::alloc(int n) {
  assert(n >= 0);
  return alloc<T>(static_cast<long unsigned int>(n));
}

template<class T>
inline void
Heap::free(T* b, long unsigned int n) {
  for (long unsigned int i=n; i--; )
    b[i].~T();
  rfree(b);
}
template<class T>
inline void
Heap::free(T* b, long int n) {
  assert(n >= 0);
  free<T>(b, static_cast<long unsigned int>(n));
}
template<class T>
inline void
Heap::free(T* b, unsigned int n) {
  free<T>(b, static_cast<long unsigned int>(n));
}
template<class T>
inline void
Heap::free(T* b, int n) {
  assert(n >= 0);
  free<T>(b, static_cast<long unsigned int>(n));
}

template<class T>
inline T*
Heap::realloc(T* b, long unsigned int n, long unsigned int m) {
  if (n == m)
    return b;
  T* p = static_cast<T*>(ralloc(sizeof(T)*m));
  for (long unsigned int i=std::min(n,m); i--; )
    (void) new (p+i) T(b[i]);
  for (long unsigned int i=n; i<m; i++)
    (void) new (p+i) T();
  free<T>(b,n);
  return p;
}
template<class T>
inline T*
Heap::realloc(T* b, long int n, long int m) {
  assert((n >= 0) && (m >= 0));
  return realloc<T>(b,static_cast<long unsigned int>(n),
                    static_cast<long unsigned int>(m));
}
template<class T>
inline T*
Heap::realloc(T* b, unsigned int n, unsigned int m) {
  return realloc<T>(b,static_cast<long unsigned int>(n),
                    static_cast<long unsigned int>(m));
}
template<class T>
inline T*
Heap::realloc(T* b, int n, int m) {
  assert((n >= 0) && (m >= 0));
  return realloc<T>(b,static_cast<long unsigned int>(n),
                    static_cast<long unsigned int>(m));
}

#define GECODE_SUPPORT_REALLOC(T)                                       \
template<>                                                            \
inline T*                                                        \
Heap::realloc<T>(T* b, long unsigned int, long unsigned int m) {      \
  return static_cast<T*>(rrealloc(b,m*sizeof(T)));                    \
}                                                                     \
template<>                                                            \
inline T*                                                        \
Heap::realloc<T>(T* b, long int n, long int m) {                      \
  assert((n >= 0) && (m >= 0));                                       \
  return realloc<T>(b,static_cast<long unsigned int>(n),              \
                    static_cast<long unsigned int>(m));               \
}                                                                     \
template<>                                                            \
inline T*                                                        \
Heap::realloc<T>(T* b, unsigned int n, unsigned int m) {              \
  return realloc<T>(b,static_cast<long unsigned int>(n),              \
                    static_cast<long unsigned int>(m));               \
}                                                                     \
template<>                                                            \
inline T*                                                        \
Heap::realloc<T>(T* b, int n, int m) {                                \
  assert((n >= 0) && (m >= 0));                                       \
  return realloc<T>(b,static_cast<long unsigned int>(n),              \
                    static_cast<long unsigned int>(m));               \
}

GECODE_SUPPORT_REALLOC(bool)
GECODE_SUPPORT_REALLOC(signed char)
GECODE_SUPPORT_REALLOC(unsigned char)
GECODE_SUPPORT_REALLOC(signed short int)
GECODE_SUPPORT_REALLOC(unsigned short int)
GECODE_SUPPORT_REALLOC(signed int)
GECODE_SUPPORT_REALLOC(unsigned int)
GECODE_SUPPORT_REALLOC(signed long int)
GECODE_SUPPORT_REALLOC(unsigned long int)
GECODE_SUPPORT_REALLOC(float)
GECODE_SUPPORT_REALLOC(double)

#undef GECODE_SUPPORT_REALLOC

template<class T>
inline T**
Heap::realloc(T** b, long unsigned int, long unsigned int m) {
  return static_cast<T**>(rrealloc(b,m*sizeof(T*)));
}
template<class T>
inline T**
Heap::realloc(T** b, long int n, long int m) {
  assert((n >= 0) && (m >= 0));
  return realloc<T*>(b,static_cast<long unsigned int>(n),
                     static_cast<long unsigned int>(m));
}
template<class T>
inline T**
Heap::realloc(T** b, unsigned int n, unsigned int m) {
  return realloc<T*>(b,static_cast<long unsigned int>(n),
                     static_cast<long unsigned int>(m));
}
template<class T>
inline T**
Heap::realloc(T** b, int n, int m) {
  assert((n >= 0) && (m >= 0));
  return realloc<T*>(b,static_cast<long unsigned int>(n),
                     static_cast<long unsigned int>(m));
}

template<class T>
inline T*
Heap::copy(T* d, const T* s, long unsigned int n) {
  for (long unsigned int i=n; i--; )
    d[i]=s[i];
  return d;
}
template<class T>
inline T*
Heap::copy(T* d, const T* s, long int n) {
  assert(n >= 0);
  return copy<T>(d,s,static_cast<long unsigned int>(n));
}
template<class T>
inline T*
Heap::copy(T* d, const T* s, unsigned int n) {
  return copy<T>(d,s,static_cast<long unsigned int>(n));
}
template<class T>
inline T*
Heap::copy(T* d, const T* s, int n) {
  assert(n >= 0);
  return copy<T>(d,s,static_cast<long unsigned int>(n));
}

#define GECODE_SUPPORT_COPY(T)                                  \
template<>                                                    \
inline T*                                                \
Heap::copy(T* d, const T* s, long unsigned int n) {           \
  return static_cast<T*>(memcpy(d,s,n*sizeof(T)));            \
}                                                             \
template<>                                                    \
inline T*                                                \
Heap::copy(T* d, const T* s, long int n) {                    \
  assert(n >= 0);                                             \
  return copy<T>(d,s,static_cast<long unsigned int>(n));      \
}                                                             \
template<>                                                    \
inline T*                                                \
Heap::copy(T* d, const T* s, unsigned int n) {                \
  return copy<T>(d,s,static_cast<long unsigned int>(n));      \
}                                                             \
template<>                                                    \
inline T*                                                \
Heap::copy(T* d, const T* s, int n) {                         \
  assert(n >= 0);                                             \
  return copy<T>(d,s,static_cast<long unsigned int>(n));      \
}

GECODE_SUPPORT_COPY(bool)
GECODE_SUPPORT_COPY(signed char)
GECODE_SUPPORT_COPY(unsigned char)
GECODE_SUPPORT_COPY(signed short int)
GECODE_SUPPORT_COPY(unsigned short int)
GECODE_SUPPORT_COPY(signed int)
GECODE_SUPPORT_COPY(unsigned int)
GECODE_SUPPORT_COPY(signed long int)
GECODE_SUPPORT_COPY(unsigned long int)
GECODE_SUPPORT_COPY(float)
GECODE_SUPPORT_COPY(double)

#undef GECODE_SUPPORT_COPY

template<class T>
inline T**
Heap::copy(T** d, const T** s, long unsigned int n) {
  return static_cast<T**>(memcpy(d,s,n*sizeof(T*)));
}
template<class T>
inline T**
Heap::copy(T** d, const T** s, long int n) {
  assert(n >= 0);
  return copy<T*>(d,s,static_cast<long unsigned int>(n));
}
template<class T>
inline T**
Heap::copy(T** d, const T** s, unsigned int n) {
  return copy<T*>(d,s,static_cast<long unsigned int>(n));
}
template<class T>
inline T**
Heap::copy(T** d, const T** s, int n) {
  assert(n >= 0);
  return copy<T*>(d,s,static_cast<long unsigned int>(n));
}

#endif // HEAP_HPP

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

#ifndef NODE_HH
#define NODE_HH

#include <cassert>
#include <QHash>

class VisualNode;

#include "heap.hpp"

class NodeAllocator;

/// \brief Base class for nodes of the search tree
class Node {
private:
  /// Tags that are used to encode the number of children
  enum {
    UNDET, //< Number of children not determined
    LEAF,  //< Leaf node
    TWO_CHILDREN, //< Node with at most two children
    MORE_CHILDREN //< Node with more than two children
  };

  /// The children, or in case there are at most two, the first child
  void* childrenOrFirstChild;

  /// The parent of this node, or nullptr for the root
  int parent;

  /// Read the tag of childrenOrFirstChild
  unsigned int getTag(void) const;
  /// Set the tag of childrenOrFirstChild for the first time
  void setTag(unsigned int tag);
  /// Reset the tag of childrenOrFirstChild (no checking of the previous tag)
  void resetTag(unsigned int tag);
  /// Return childrenOrFirstChild without tag
  void* getPtr(void) const;
  /// Return childrenOrFirstChild as integer
  int getFirstChild(void) const;
  /// Change the content of childrenOrFirstChild
  void setFirstChild(int n);

protected:

  /** The number of children, in case it is greater than 2, or the first
   *  child (if negative)
   */
  int noOfChildren;

  /// Return whether this node is undetermined
  bool isUndetermined(void) const;

  /// Return index of child no \a n

public:

  /// Construct node with parent \a p
  Node(int p, bool failed = false);

  void replaceChild(NodeAllocator& na, int new_gid, int alt);

  int getChild(int n) const;
  /// Return the parent
  int getParent(void) const;
  /// Return the parent
  VisualNode* getParent(const NodeAllocator& na) const;
  /// Return child no \a n
  VisualNode* getChild(const NodeAllocator& na, int n) const;

  /// Return index of this node
  int getIndex(const NodeAllocator& na) const;

  /// Check if this node is the root of a tree
  bool isRoot(void) const;

  /// Set the number of children to \a n and initialize children
  void setNumberOfChildren(unsigned int n, NodeAllocator& na);

  /// Add uninitialised child and return it
  int addChild(NodeAllocator& na);

  /// Return the number of children
  unsigned int getNumberOfChildren(void) const;

  void removeChild(int n);

  /// for multithreaded search (drawing)
  /// thread id
  char _tid = 0;  // TODO: take out of Node class; assigned in treebuilder

#ifdef MAXIM_DEBUG
  int debug_id;
  static int debug_instance_counter;

#endif

};

#endif // NODE_HH

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GECODE_GIST_NODEVISITOR_HH
#define GECODE_GIST_NODEVISITOR_HH

/// \brief Base class for a visitor that runs a cursor over a tree
template<class Cursor>
class NodeVisitor {
protected:
    /// The cursor
    Cursor c;
public:
    /// Constructor
    NodeVisitor(const Cursor& c0);
    /// Reset the cursor object to \a c0
    void setCursor(const Cursor& c0);
    /// Return the cursor
    Cursor& getCursor(void);
};

/// \brief Run a cursor over a tree, processing nodes in post-order
template<class Cursor>
class PostorderNodeVisitor : public NodeVisitor<Cursor> {
protected:
    using NodeVisitor<Cursor>::c;
    /// Move the cursor to the left-most leaf
    void moveToLeaf(void);
public:
    /// Constructor
    PostorderNodeVisitor(const Cursor& c);
    /// Move cursor to next node, return true if succeeded
    bool next(void);
    /// Execute visitor
    void run(void);
};

/// \brief Run a cursor over a tree, processing nodes in pre-order
template<class Cursor>
class PreorderNodeVisitor : public NodeVisitor<Cursor> {
protected:
    using NodeVisitor<Cursor>::c;
    /// Move cursor to next node from a leaf
    bool backtrack(void);
public:
    /// Constructor
    PreorderNodeVisitor(const Cursor& c);
    /// Move cursor to the next node, return true if succeeded
    bool next(void);
    /// Execute visitor
    void run(void);
};

/// \brief Run a cursor through ancestors of the node
template<class Cursor>
class AncestorNodeVisitor : public NodeVisitor<Cursor> {
protected:
    using NodeVisitor<Cursor>::c;
public:
    /// Constructor
    AncestorNodeVisitor(const Cursor& c);
    /// Move cursor to the next node, return true if succeeded
    bool next(void);
    /// Execute visitor
    void run(void);
};


#include "nodevisitor.hpp"

#endif

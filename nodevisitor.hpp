
template<class Cursor>
inline
NodeVisitor<Cursor>::NodeVisitor(const Cursor& c0) : c(c0) {}

template<class Cursor>
inline void
NodeVisitor<Cursor>::setCursor(const Cursor& c0) { c = c0; }

template<class Cursor>
inline Cursor&
NodeVisitor<Cursor>::getCursor(void) { return c; }

template<class Cursor>
inline void
PostorderNodeVisitor<Cursor>::moveToLeaf(void) {
    while (c.mayMoveDownwards()) {
        c.moveDownwards();
    }
}

template<class Cursor>
PostorderNodeVisitor<Cursor>::PostorderNodeVisitor(const Cursor& c0)
    : NodeVisitor<Cursor>(c0) {
    moveToLeaf();
}

template<class Cursor>
inline bool
PostorderNodeVisitor<Cursor>::next(void) {
    c.processCurrentNode();
    if (c.mayMoveSidewards()) {
        c.moveSidewards();
        moveToLeaf();
    } else if (c.mayMoveUpwards()) {
        c.moveUpwards();
    } else {
        return false;
    }
    return true;
}

template<class Cursor>
inline void
PostorderNodeVisitor<Cursor>::run(void) {
    while (next()) {}
}

template<class Cursor>
inline bool
PreorderNodeVisitor<Cursor>::backtrack(void) {
    while (! c.mayMoveSidewards() && c.mayMoveUpwards()) {
        c.moveUpwards();
    }
    if (! c.mayMoveUpwards()) {
        return false;
    } else {
        c.moveSidewards();
    }
    return true;
}

template<class Cursor>
PreorderNodeVisitor<Cursor>::PreorderNodeVisitor(const Cursor& c0)
    : NodeVisitor<Cursor>(c0) {}

template<class Cursor>
inline bool
PreorderNodeVisitor<Cursor>::next(void) {
    c.processCurrentNode();
    if (c.mayMoveDownwards()) {
        c.moveDownwards();
    } else if (c.mayMoveSidewards()) {
        c.moveSidewards();
    } else {
        return backtrack();
    }
    return true;
}

template<class Cursor>
inline void
PreorderNodeVisitor<Cursor>::run(void) {
    while (next()) {}
}

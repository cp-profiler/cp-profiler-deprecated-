/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#ifndef NODETREE_HH
#define NODETREE_HH

#include <QMutex>
#include <QObject>
#include "visualnode.hh"

class NodeTree : public QObject {
Q_OBJECT
private:
    /// Mutex for synchronizing acccess to the tree
    QMutex treeMutex {QMutex::Recursive};
    /// This should be a part of the `Visual Tree`
    /// Mutex for synchronizing layout and drawing
    QMutex layoutMutex {QMutex::Recursive};
    NodeAllocator na;
    Statistics stats;
public:
    NodeTree();
    ~NodeTree();

    VisualNode* getChild(const VisualNode& node, int alt);
    const VisualNode* getChild(const VisualNode& node, int alt) const;

    const NodeAllocator& getNA() const;
    NodeAllocator& getNA();

    const VisualNode* getRoot() const;
    VisualNode* getRoot();

    const VisualNode* getNode(int gid) const;
    VisualNode* getNode(int gid);

    int getIndex(const VisualNode*) const;

    const Statistics& getStatistics() const;
    Statistics& getStatistics();

    QMutex& getTreeMutex();
    QMutex& getLayoutMutex();

private:
signals:
    void treeModified();
};

#endif
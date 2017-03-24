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
 */

#include "nodetree.hh"

NodeTree::NodeTree()
    : mutex(QMutex::Recursive), layoutMutex(QMutex::Recursive) {
  int rootIdx = na.allocateRoot();
  assert(rootIdx == 0);
  (void)rootIdx;
  na[0]->setMarked(true);
}

NodeTree::~NodeTree() = default;

const NodeAllocator& NodeTree::getNA() const { return na; }
NodeAllocator& NodeTree::getNA() { return na; }

const VisualNode* NodeTree::getRoot() const { return na[0]; }
VisualNode* NodeTree::getRoot() { return na[0]; }

const VisualNode* NodeTree::getNode(int gid) const { return na[gid]; }
VisualNode* NodeTree::getNode(int gid) { return na[gid]; }

const VisualNode* NodeTree::getChild(const VisualNode& node, int alt) const {
    return na[node.getChild(alt)];
}

VisualNode* NodeTree::getChild(const VisualNode& node, int alt) {
    return na[node.getChild(alt)];
}

int NodeTree::getIndex(const VisualNode* node) const {
  return node->getIndex(na);
}

Statistics& NodeTree::getStatistics() {
    return stats;
}

const Statistics& NodeTree::getStatistics() const {
    return stats;
}

QMutex& NodeTree::getMutex() { return mutex; }
QMutex& NodeTree::getLayoutMutex() { return layoutMutex; }
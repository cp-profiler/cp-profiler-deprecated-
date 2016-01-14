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

#include "backjumps.hh"
#include "nodevisitor.hh"

#include <iostream>

using namespace cpprofiler::analysis;
using std::cout;

Backjumps::Backjumps() {}

void Backjumps::findBackjumps(VisualNode* root, const VisualNode::NodeAllocator& na) {

  BackjumpsCursor bj_cursor(root, na);
  PreorderNodeVisitor<BackjumpsCursor> visitor(bj_cursor);

  visitor.run();

}

BackjumpsCursor::BackjumpsCursor(VisualNode* root,
  const VisualNode::NodeAllocator& na)
  : NodeCursor<VisualNode>(root,na) {

}

void
BackjumpsCursor::moveDownwards(void) {
    ++cur_level;
    NodeCursor<VisualNode>::moveDownwards();
}


void
BackjumpsCursor::moveUpwards(void) {
    --cur_level;
    NodeCursor<VisualNode>::moveUpwards();
}

void
BackjumpsCursor::processCurrentNode(void) {
  auto n = node();
  auto status = n->getStatus();

  if (status == NodeStatus::FAILED || status == NodeStatus::SOLVED) {
    last_failure_level = cur_level;
  }

  if (status == NodeStatus::SKIPPED) {

    ++skipped_count;

    if (!is_backjumping) { 
      /// Backjump starts (form the last failure node)
      is_backjumping = true;
      cout << "Backjump from level: " << last_failure_level;
    }

  } else {

    if (is_backjumping) {
      is_backjumping = false;
      /// One level above a non-skipped node (branch node)
      cout << " to: " << cur_level - 1 << " skipping: " << skipped_count << std::endl;
      skipped_count = 0;
    }

  }

}
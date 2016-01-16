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

const std::unordered_map<uint, BackjumpItem>
Backjumps::findBackjumps(VisualNode* root, const VisualNode::NodeAllocator& na) {

  std::unordered_map<uint, BackjumpItem> bj_data;

  BackjumpsCursor bj_cursor(root, na, bj_data);
  PreorderNodeVisitor<BackjumpsCursor> visitor(bj_cursor);

  visitor.run();

  return bj_data;

}

BackjumpsCursor::BackjumpsCursor(VisualNode* root,
  const VisualNode::NodeAllocator& na,
  std::unordered_map<uint, BackjumpItem>& bj_data)
  : NodeCursor<VisualNode>(root,na), bj_data(bj_data) {

}

void
BackjumpsCursor::moveDownwards() {
    ++cur_level;
    NodeCursor<VisualNode>::moveDownwards();
}


void
BackjumpsCursor::moveUpwards() {
    --cur_level;
    NodeCursor<VisualNode>::moveUpwards();
}

void
BackjumpsCursor::processCurrentNode() {
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
      bj_item.level_from = last_failure_level;
      cout << "Backjump from level: " << last_failure_level;
    }

  } else {

    if (is_backjumping) {
      is_backjumping = false;
      /// One level above a non-skipped node (branch node)
      cout << " to: " << cur_level - 1 << " skipping: " << skipped_count << std::endl;
      bj_item.level_to = cur_level - 1;
      bj_item.nodes_skipped = skipped_count;
      bj_data[n->getIndex(na)] = bj_item;
      skipped_count = 0;
    }

  }

}
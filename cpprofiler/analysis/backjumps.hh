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

#ifndef CPPROFILER_ANALYSIS_BACKJUMPS_HH
#define CPPROFILER_ANALYSIS_BACKJUMPS_HH

#include "nodecursor.hh"

class VisualNode;

namespace cpprofiler {
namespace analysis {

struct BackjumpItem {
  int level_from;
  int level_to;
  int nodes_skipped;
};

struct BackjumpData {
  std::unordered_map<uint, BackjumpItem> bj_map;
  int max_from = 0;
  int max_to = 0;
  int max_skipped = 0;
};

class Backjumps {
 public:
  Backjumps();

  const BackjumpData findBackjumps(VisualNode* root, const NodeAllocator& na);
};

/// A cursor that prints backjumps
class BackjumpsCursor : public NodeCursor<VisualNode> {
 private:
  int cur_level = 0;  /// depth
  int last_failure_level =
      0;  /// can be a solution as well (for the purpose of bj)
  bool is_backjumping =
      false;  /// whether the cursor on a backjumped part of the tree
  int skipped_count = 0;  /// counts how many skipped nodes for every bj
  int bj_gid = 0;         /// gid of a node the current backjump started from
  /// temp bj_item to be copied into the map after having been constructed
  BackjumpItem bj_item;

  /// contains a map from gid to backjump item
  BackjumpData& bj_data;

 public:
  BackjumpsCursor(VisualNode* theNode, const NodeAllocator& na,
                  BackjumpData& bj_data);

  void processCurrentNode();
  void moveDownwards();
  void moveUpwards();
};
}
}

#endif
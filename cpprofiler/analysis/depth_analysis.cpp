#include "depth_analysis.hh"
#include <QDebug>
#include "treecanvas.hh"

using namespace cpprofiler::analysis;

DepthAnalysis::DepthAnalysis(TreeCanvas& tc) : _tc(tc), _na(tc.get_na()) {}

std::vector<Direction> DepthAnalysis::collectDepthData() {
  SpaceNode* root = (*_na)[0];
  std::vector<Direction> depth_data;
  depth_data.reserve(_tc.getExecution()->getData()->size());

  traverse(depth_data, root);

  return depth_data;
}

void DepthAnalysis::traverse(std::vector<Direction>& depth_data,
                             const SpaceNode* const n) {
  for (unsigned i = 0; i < n->getNumberOfChildren(); i++) {
    depth_data.push_back(Direction::DOWN);
    traverse(depth_data, n->getChild(*_na, i));
  }

  if (n->getStatus() == NodeStatus::SOLVED) {
    depth_data.push_back(Direction::SOLUTION);
  }

  /// slightly different behaviour from the root node
  if (n->getParent() >= 0) {
    depth_data.push_back(Direction::UP);
  }
}

std::vector<std::vector<unsigned int> > DepthAnalysis::runMSL() {
  using std::vector;

  vector<Direction> depth_data = collectDepthData();

  unsigned int deepest = 0;
  unsigned int curr_level = 1;
  unsigned int total_depth = _tc.getTreeDepth();
  vector<unsigned int> dl_list(total_depth);  /// deepest at level
  vector<unsigned int> count_list(total_depth, 0);

  vector<vector<unsigned int> > count_array(total_depth);  /// history of count
  for (auto v : count_array) {
    v.reserve(depth_data.size());
  }

  assert(depth_data[0] ==
         Direction::DOWN);  /// otherwise initial curr_level isn't 1

  /// initialize dl_list with arr[i] = i
  for (unsigned int i = 0; i < dl_list.size(); i++) {
    dl_list[i] = i;
  }

  /// traverse the vector in pairs (i - 1; i)
  for (unsigned int i = 1; i < depth_data.size(); i++) {
    Direction prev = depth_data[i - 1];
    Direction curr = depth_data[i];

    /// Reset count list and continue
    if (curr == Direction::SOLUTION) {
      for (auto& c : count_list) {
        c = 0;
      }
      continue;
    }

    auto prev_level = curr_level;

    /// update current level value
    if (curr == Direction::DOWN)
      curr_level++;
    else if (curr == Direction::UP)
      curr_level--;

    /// NAV backtrack (No Assigned Value)
    if (prev == Direction::DOWN && curr == Direction::UP) {
      assert(deepest <= dl_list[prev_level]);
      deepest = prev_level;  /// only NAV changes `deepest` and always does so
    }

    /// USS (Unsuccessful Subspace Search)
    if (curr == Direction::UP) {
      if (deepest == dl_list[curr_level]) {
        count_list[curr_level] += 1;
        /// the paper checks against some threshold here
      } else if (deepest > dl_list[curr_level]) {
        count_list[curr_level] = 1;
        dl_list[curr_level] = deepest;
      }
    }

    /// SAV (Successfully Assigned Values)
    if (prev == Direction::UP && curr == Direction::UP) {
      assert(deepest <= dl_list[prev_level]);
      if (deepest < dl_list[prev_level]) {
        count_list[prev_level] = 0;
        dl_list[prev_level] = deepest;
      }
    }

    /// copy count_list to count_array (only when leaving a node):
    if (curr == Direction::UP) {
      for (unsigned d = 0; d < total_depth; d++) {
        // count_array[d][i] = count_list[d];
        count_array.at(d).push_back(count_list[d]);
      }
    }
  }

  return count_array;
}

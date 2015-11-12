#include "depth_analysis.hh"
#include <QDebug>

DepthAnalysisDialog::DepthAnalysisDialog(TreeCanvas* tc, QWidget* parent)
  :_tc(tc), _na(tc->na), QDialog(parent) {
    
    std::vector<Direction> depth_data = collectDepthData();

    runMSL(depth_data);
}

unsigned int DepthAnalysisDialog::getTotalDepth() {

}

std::vector<Direction> DepthAnalysisDialog::collectDepthData() {

  SpaceNode* root = (*_na)[0];
  std::vector<Direction> depth_data;
  depth_data.reserve(_tc->getData()->size());

  traverse(depth_data, root);

  return depth_data;
}

void DepthAnalysisDialog::traverse(std::vector<Direction>& depth_data, const SpaceNode* const n) {
  for (unsigned int i = 0; i < n->getNumberOfChildren(); i++) {
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

void DepthAnalysisDialog::runMSL(const std::vector<Direction>& depth_data) {

  unsigned int deepest = 0;
  unsigned int curr_level = 1;
  unsigned int total_depth = _tc->getTreeDepth();
  std::vector<unsigned int> dl_list(total_depth); /// deepest at level
  std::vector<unsigned int> count_list(total_depth, 0); /// deepest at level

  assert(depth_data[0] == Direction::DOWN); /// otherwise initial curr_level isn't 1

  /// initialize dl_list with arr[i] = i
  for (unsigned int i = 0; i < dl_list.size(); i++) { dl_list[i] = i; }

  /// traverse the vector in pairs (i - 1; i)
  for (unsigned int i = 1; i < depth_data.size(); i++) {
    Direction prev = depth_data[i - 1];
    Direction curr = depth_data[i];

    /// Reset count list and continue
    if (curr == Direction::SOLUTION) {
      for (auto& c : count_list) { c = 0; }
      continue;
    }

    /// update current level value
    if (curr == Direction::DOWN)
      curr_level++;
    else if (curr == Direction::UP)
      curr_level--;

    /// NAV backtrack (No Assigned Value)
    if (prev == Direction::DOWN && curr == Direction::UP) {
      assert(deepest <= dl_list[curr_level]);
      deepest = curr_level; /// only NAV changes `deepest` and always does so

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
      count_list[curr_level] = 0;
      dl_list[curr_level] = deepest;
    }

  }
}
#ifndef DEPTH_ANALYSIS_HH
#define DEPTH_ANALYSIS_HH

#include <QDialog>
#include <vector>
#include "treecanvas.hh"

enum Direction { DOWN, UP, SOLUTION };

class DepthAnalysisDialog : public QDialog {

private:
  TreeCanvas* _tc;
  NodeAllocator*  _na;

  unsigned int _total_depth;


private:

  /// returns a vector of 'UPs' and 'DOWNs'
  std::vector<Direction> collectDepthData();
  unsigned int getTotalDepth();

  void traverse(std::vector<Direction>& depth_data, const SpaceNode* const n);

  void runMSL(const std::vector<Direction>& depth_data);

public:
  DepthAnalysisDialog(TreeCanvas* tc, QWidget* parent);

};

#endif // DEPTH_ANALYSIS_HH

#ifndef DEPTH_ANALYSIS_HH
#define DEPTH_ANALYSIS_HH

#include <QDialog>
#include <vector>
#include "treecanvas.hh"

enum Direction { DOWN, UP, SOLUTION };

class DepthAnalysis : public QObject {
  Q_OBJECT

private:
  TreeCanvas& _tc;
  NodeAllocator*  _na;

  unsigned int _total_depth;


private:

  /// returns a vector of 'UPs' and 'DOWNs'
  std::vector<Direction> collectDepthData();

  void traverse(std::vector<Direction>& depth_data, const SpaceNode* const n);


public:
  DepthAnalysis(TreeCanvas& tc);

  /// Return a two-dimentional array of counts
  std::vector< std::vector<unsigned int> > runMSL();

};

#endif DEPTH_ANALYSIS_HH
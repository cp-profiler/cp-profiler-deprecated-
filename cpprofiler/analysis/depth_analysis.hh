#ifndef DEPTH_ANALYSIS_HH
#define DEPTH_ANALYSIS_HH

#include <QDialog>
#include <vector>

class TreeCanvas;
class VisualNode;
class SpaceNode;
class NodeAllocator;

class depth_analysis {
 public:
  depth_analysis();
  ~depth_analysis();
};

namespace cpprofiler {
namespace analysis {

class DepthAnalysis : public QObject {
  Q_OBJECT

 private:
  TreeCanvas& _tc;
  NodeAllocator& _na;

  unsigned int _total_depth;

 private:
  /// returns a vector of 'UPs' and 'DOWNs'
  std::vector<Direction> collectDepthData();

  void traverse(std::vector<Direction>& depth_data, const SpaceNode* const n);

 public:
  explicit DepthAnalysis(TreeCanvas& tc);

  /// Return a two-dimentional array of counts
  std::vector<std::vector<unsigned int> > runMSL();
};
}
}

#endif

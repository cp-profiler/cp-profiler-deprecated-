#ifndef DEPTH_ANALYSIS_HH
#define DEPTH_ANALYSIS_HH

#include <QDialog>
#include <vector>

class NodeTree;
class SpaceNode;
class NodeAllocator;

class depth_analysis {
 public:
  depth_analysis();
  ~depth_analysis();
};

namespace cpprofiler {
namespace analysis {

enum class Direction { DOWN, UP, SOLUTION };

class DepthAnalysis : public QObject {
  Q_OBJECT

 private:
  NodeTree& _nt;
  NodeAllocator& _na;

  unsigned int _total_depth;

  /// returns a vector of 'UPs' and 'DOWNs'
  std::vector<Direction> collectDepthData();

  void traverse(std::vector<Direction>& depth_data, const SpaceNode* const n);

 public:
  explicit DepthAnalysis(NodeTree& nt);

  /// Return a two-dimentional array of counts
  std::vector<std::vector<unsigned int> > runMSL();
};
}
}

#endif

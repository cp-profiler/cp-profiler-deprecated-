#ifndef SIMILAR_SHAPES_HH
#define SIMILAR_SHAPES_HH

#include <memory>
#include <unordered_map>


#include <QDebug>
#include <QLabel>

#include "histogram_win.hh"
#include "subtree_analysis.hh"

class VisualNode;

class QGraphicsScene;
class QGraphicsView;
class QAbstractScrollArea;
class Execution;
class NodeTree;

namespace cpprofiler {
namespace analysis {

class SubtreeCanvas;
class SimilarShapesWindow;

struct SubtreeInfo {
  std::vector<VisualNode*> nodes;
  int size;
  int height;
  int count;
  bool marked;

  SubtreeInfo(std::vector<VisualNode*> ns, int sz, int ht, int ct, bool mk)
  : nodes{ns}, size{sz}, height{ht}, count{ct}, marked{mk} {}

  int get_count() const { return count; }
};

class Filters {
public:
    int minDepth = 2;
    int minCount = 2;

    bool check(int depth, int count) const {
      if (depth < minDepth) return false;
      if (count < minCount) return false;
      return true;
    }

};


class SimilarShapesWindow : public HistogramWindow {
  Q_OBJECT

  ShapeProperty   m_histType  = ShapeProperty::SIZE;
  ShapeProperty   m_sortType  = ShapeProperty::SIZE;

  Filters filters;

  void initInterface();

  /// Reset the scene and call drawHistorgram/drawAlternativeHistogram
  void updateHistogram();

  void drawAnalysisHistogram(ShapeProperty prop, std::vector<SubtreeInfo>& vec);

  void drawHistogram();
  void drawAlternativeHistogram();

 public:
  SimilarShapesWindow(Execution& nt);
  ~SimilarShapesWindow();


};




}
}

#endif

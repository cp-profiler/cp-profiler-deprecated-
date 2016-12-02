#ifndef SIMILAR_SHAPES_HH
#define SIMILAR_SHAPES_HH

#include <memory>
#include <unordered_map>

#include <QDialog>
#include <QDebug>
#include <QLabel>

class VisualNode;
class Shape;

class QGraphicsScene;
class QGraphicsView;
class QAbstractScrollArea;
class NodeTree;

namespace cpprofiler {
namespace analysis {

class SubtreeCanvas;
class SimilarShapesWindow;

struct ShapeInfo {
  int sol;
  int size;
  int height;
  std::vector<VisualNode*> nodes;
  Shape* s;
};

struct SubtreeInfo {
  VisualNode* node;
  int size;
  int height;
  int count;
  bool marked;

  int get_count() const { return count; }
};

/// used for comparison
struct SubtreeInfo2 {
    VisualNode* node; /// any node from the corresponding group
    int size;
    int height;
    int count_ex1;
    int count_ex2;

    int get_count() const { return count_ex1 + count_ex2; }
};

class Filters {
    int m_minDepth = 2;
    int m_minCount = 2;

   public:
    Filters();
    void setMinDepth(int);
    void setMinCount(int);
    bool check(int depth, int count);
    
  };

enum class ShapeProperty { SIZE, COUNT, HEIGHT };

using GroupsOfNodes_t = std::vector<std::vector<VisualNode*>>;

enum class SimilarityType {
  SHAPE, SUBTREE
};

enum class AnalysisType {
  SINGLE, COMPARISON
};


class SimilarShapesWindow : public QDialog {
  Q_OBJECT

  NodeTree& node_tree;

  SimilarityType  simType     = SimilarityType::SUBTREE;
  ShapeProperty   m_histType  = ShapeProperty::SIZE;
  ShapeProperty   m_sortType  = ShapeProperty::SIZE;

  AnalysisType    m_type;

  // only used for AnalysisType::COMPARISON
  std::unique_ptr<std::unordered_map<VisualNode*, int>> node2ex_id;

  bool labelSensitive         = false;

  bool shapes_cached    = false;
  bool subtrees_cached  = false;

  std::unique_ptr<SubtreeCanvas> m_SubtreeCanvas;
  std::unique_ptr<QGraphicsScene> m_scene;

  /// the result of similar shapes analysis
  std::vector<ShapeInfo> shapes;

  /// the result of identical subree analysis
  GroupsOfNodes_t m_identicalGroups;

  QAbstractScrollArea* m_scrollArea;
  QGraphicsView* hist_view;
  Filters filters;


#ifdef MAXIM_DEBUG
  QLabel debug_label{"debug info"};
#endif

  void initInterface(QAbstractScrollArea* sa);

  /// Reset the scene and call drawHistorgram/drawAlternativeHistogram
  void updateHistogram();
  void drawHistogram();
  void drawAlternativeHistogram();
  void drawComparisonHistogram();

  /// runs similar shapes analysis and writes the result to `shapes`
  void updateShapesData();


 public:
  SimilarShapesWindow(NodeTree& nt);
  SimilarShapesWindow(NodeTree& nt, std::unique_ptr<std::unordered_map<VisualNode*, int>>);
  ~SimilarShapesWindow();
  
  /// Called from a rectangle representing a shape
  void highlightSubtrees(VisualNode* n);

};




}
}

#endif

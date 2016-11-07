#ifndef SIMILAR_SHAPES_HH
#define SIMILAR_SHAPES_HH

#include <QDialog>
#include <QDebug>
#include <QLabel>
#include <set>
#include <memory>

#include <QGraphicsScene>
#include <QGraphicsRectItem>


class TreeCanvas;
class VisualNode;
class Shape;

class QAbstractScrollArea;
class NodeTree;

namespace cpprofiler {
namespace analysis {
class ShapeCanvas;
class SimilarShapesWindow;

/// for temporary stuff
struct ShapeI {
  int sol;
  int shape_size;
  int shape_height;
  VisualNode* node;
  Shape* s;
  ShapeI(int sol0, VisualNode* node0);
  ~ShapeI();
  ShapeI(const ShapeI& sh);
  ShapeI& operator=(const ShapeI& other);
};

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
};

namespace detail {

  struct FiltersInfo {
    int height;
    int count;
  };

  class Filters {
    int m_minDepth = 2;
    int m_minCount = 2;

   public:
    Filters();
    void setMinDepth(int);
    void setMinCount(int);
    bool apply(const ShapeInfo& s);
    
  };
}



/// less operator needed for the map
struct CompareShapes {
 public:
  bool operator()(const ShapeI& s1, const ShapeI& s2) const;
};

enum class ShapeProperty { SIZE, COUNT, HEIGHT };

using GroupsOfNodes_t = std::vector<std::vector<VisualNode*>>;

enum class SimilarityType {
  SHAPE, SUBTREE
};


class SimilarShapesWindow : public QDialog {
  Q_OBJECT

  friend class detail::Filters;
  friend class ::TreeCanvas;

  TreeCanvas& m_tc; /// for highlighting subtrees
  NodeTree& node_tree;

  SimilarityType  simType     = SimilarityType::SHAPE;
  ShapeProperty   m_histType  = ShapeProperty::SIZE;
  ShapeProperty   m_sortType  = ShapeProperty::SIZE;

  std::unique_ptr<ShapeCanvas> m_ShapeCanvas;
  std::unique_ptr<QGraphicsScene> m_scene;

  /// the result of similar shapes analysis
  std::vector<ShapeInfo> shapes;

  /// the result of identical subrees analysis
  GroupsOfNodes_t m_identicalGroups;

  QAbstractScrollArea* m_scrollArea;
  QGraphicsView* hist_view;
  detail::Filters filters;


#ifdef MAXIM_DEBUG
  QLabel debug_label{"debug info"};
#endif

  /// Loop through all nodes and add them to the multimap
  /// Called on construction
  std::multiset<ShapeI, CompareShapes> collectSimilarShapes();

  void initInterface();

  /// Reset the scene and call drawHistorgram/drawAlternativeHistogram
  void updateHistogram();
  void drawHistogram();
  void drawAlternativeHistogram();

 public:
  SimilarShapesWindow(TreeCanvas* tc, NodeTree& nt);
  
  /// Called from a rectangle representing a shape
  void highlightSubtrees(VisualNode* n);

};

/// "Connects" to a tree and shows a part of it
class ShapeCanvas : public QWidget {
Q_OBJECT
  const QAbstractScrollArea* m_ScrollArea;
  const NodeTree& m_NodeTree;
  VisualNode* m_targetNode = nullptr;

  void paintEvent(QPaintEvent* event) override;
public:
  ShapeCanvas(QAbstractScrollArea* parent, const NodeTree& nt);
  void showShape(VisualNode* node);
};


}
}

#endif

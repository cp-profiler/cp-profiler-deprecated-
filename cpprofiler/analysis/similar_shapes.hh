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
class SimilarShapesCursor;
class NodeTree;

namespace cpprofiler {
namespace analysis {
class ShapeCanvas;
class SimilarShapesWindow;

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
   public:
    explicit Filters(const SimilarShapesWindow& ssw);
    void setMinDepth(int);
    void setMinCount(int);
    bool apply(const ShapeI& s);
    bool apply(const FiltersInfo& s);

   private:
    int m_minDepth = 2;
    int m_minCount = 2;
    const SimilarShapesWindow& m_ssWindow;
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

  friend class ::SimilarShapesCursor;
  friend class detail::Filters;
  friend class ::TreeCanvas;

 private:

  TreeCanvas* m_tc;
  const NodeTree& node_tree;

  bool m_done = false;

  SimilarityType simType = SimilarityType::SHAPE;

  ShapeCanvas* shapeCanvas;
  std::multiset<ShapeI, CompareShapes> shapeSet;
  std::vector<ShapeI> shapesShown;

  QAbstractScrollArea* m_scrollArea;
  QGraphicsView* view;
  std::unique_ptr<QGraphicsScene> scene;
  detail::Filters filters;

  ShapeProperty m_histType = ShapeProperty::SIZE;
  ShapeProperty m_sortType = ShapeProperty::SIZE;

  GroupsOfNodes_t m_identicalGroups;

#ifdef MAXIM_DEBUG
  QLabel debug_label{"debug info"};
#endif

private:
    /// Loop through all nodes and add them to the multimap
  void collectSimilarShapes();
  void initInterface();

  void updateHistogram();
  void drawHistogram();

 public:
  SimilarShapesWindow(TreeCanvas* tc, const NodeTree& nt);
  void drawAlternativeHistogram();
  void highlightSubtrees(VisualNode* n);

 public Q_SLOTS:
  void depthFilterChanged(int val);
  void countFilterChanged(int val);


};

class ShapeCanvas : public QWidget {
Q_OBJECT
  const NodeTree& m_NodeTree;
  const QAbstractScrollArea* m_sa;
  VisualNode* m_targetNode = nullptr;

  int xtrans;

  /// paint the shape
  void paintEvent(QPaintEvent* event);
public:
  ShapeCanvas(QAbstractScrollArea* parent, const NodeTree& nt);
  void highlightShape(VisualNode* node);
};

class ShapeRect : public QGraphicsRectItem {
public:
  static constexpr int HEIGHT = 16;
  static constexpr int HALF_HEIGHT = HEIGHT / 2;
  static constexpr int PIXELS_PER_VALUE = 5;
  static constexpr int SELECTION_WIDTH = 600;

private:
  VisualNode* const m_node;
  SimilarShapesWindow* const m_ssWindow;

public:
  ShapeRect(int x, int y, int width, VisualNode*, SimilarShapesWindow*);

  VisualNode* getNode() const;
  // add to the scene
  void draw(QGraphicsScene* scene);
  QGraphicsRectItem visibleArea;

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent*);


};
}
}

#endif

#ifndef SIMILAR_SHAPES_HH
#define SIMILAR_SHAPES_HH

#include <QDialog>
#include <set>
#include <memory>

#include <QGraphicsScene>
#include <QGraphicsRectItem>

class TreeCanvas;
class VisualNode;
class Shape;

class QAbstractScrollArea;
class SimilarShapesCursor;

namespace cpprofiler {
namespace analysis {
class ShapeCanvas;
class SimilarShapesWindow;

class ShapeI {
 public:
  int sol;
  int shape_size;
  VisualNode* node;
  Shape* s;
  ShapeI(int sol0, VisualNode* node0);
  ~ShapeI();
  ShapeI(const ShapeI& sh);
  ShapeI& operator=(const ShapeI& sh);
};

class Filters {
 public:
  explicit Filters(const SimilarShapesWindow& ssw);
  void setMinDepth(int);
  void setMinCount(int);
  bool apply(const ShapeI& s);

 private:
  int m_minDepth = 2;
  int m_minCount = 2;
  const SimilarShapesWindow& m_ssWindow;
};

/// less operator needed for the map
struct CompareShapes {
 public:
  bool operator()(const ShapeI& s1, const ShapeI& s2) const;
};

enum class ShapeProperty { SIZE, OCCURRENCE };

class SimilarShapesWindow : public QDialog {
  Q_OBJECT

  friend class ::SimilarShapesCursor;
  friend class Filters;
  friend class ::TreeCanvas;

 public:
  explicit SimilarShapesWindow(TreeCanvas* tc);
  void drawHistogram();

 public Q_SLOTS:
  void depthFilterChanged(int val);
  void countFilterChanged(int val);

 private:
  /// Loop through all nodes and add them to the multimap
  void addNodesToMap();

  bool m_done = false;

  TreeCanvas* m_tc;
  ShapeCanvas* shapeCanvas;
  std::multiset<ShapeI, CompareShapes> shapesMap;

  QGraphicsView* view;
  std::unique_ptr<QGraphicsScene> scene;
  Filters filters;

  // ShapeProperty m_histType = ShapeProperty::SIZE;
  ShapeProperty m_histType = ShapeProperty::OCCURRENCE;
};

class ShapeCanvas : public QWidget {
  Q_OBJECT
 public:
  ShapeCanvas(QAbstractScrollArea* parent, TreeCanvas* tc,
              const std::multiset<ShapeI, CompareShapes>& sm);
  void highlightShape(VisualNode* node);

 private:
  const QAbstractScrollArea* m_sa;
  VisualNode* m_targetNode = nullptr;  // what is it?
  TreeCanvas* m_tc;
  // TODO(maxim): maybe this isn't necessary here
  const std::multiset<ShapeI, CompareShapes>& m_shapesMap;

  int xtrans;

  // width and height of the shape
  int width, height;

 protected:
  /// Paint the shape
  void paintEvent(QPaintEvent* event);
 protected Q_SLOTS:
  void scroll();
};

class ShapeRect : public QGraphicsRectItem {
 public:
  static constexpr int HEIGHT = 16;
  static constexpr int HALF_HEIGHT = HEIGHT / 2;
  static constexpr int PIXELS_PER_VALUE = 5;
  static constexpr int SELECTION_WIDTH = 600;

  ShapeRect(int x, int y, int width, VisualNode*, ShapeCanvas*,
            QGraphicsItem* parent = nullptr);
  VisualNode* getNode();
  // add to the scene
  void draw(QGraphicsScene* scene);
  QGraphicsRectItem visibleArea;

 protected:
  void mousePressEvent(QGraphicsSceneMouseEvent*);

 private:
  VisualNode* m_node;
  ShapeCanvas* m_canvas;
};
}
}

#endif

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


namespace cpprofiler { namespace analysis {
class ShapeCanvas;
class SimilarShapesWindow;

class ShapeI {
public:
  int sol;
  VisualNode* node;
  Shape* s;
  ShapeI(int sol0, VisualNode* node0);
  ~ShapeI();
  ShapeI(const ShapeI& sh);
  ShapeI& operator =(const ShapeI& sh);
};

class Filters {
public:
  explicit Filters(const SimilarShapesWindow& ssw);
  void setMinDepth(int);
  void setMinCount(int);
  bool apply(const ShapeI& s);
private:
  int m_minDepth = 1;
  int m_minCount = 1;
  const SimilarShapesWindow& m_ssWindow;
};

/// less operator needed for the map
struct CompareShapes {
public:
  bool operator()(const ShapeI& s1, const ShapeI& s2) const;
};



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

  TreeCanvas* m_tc;
  ShapeCanvas* shapeCanvas;
  std::multiset<ShapeI, CompareShapes> shapesMap;

  QGraphicsView* view;
  std::unique_ptr<QGraphicsScene> scene;
  Filters filters;

};

class ShapeCanvas : public QWidget {
  Q_OBJECT
public:
  ShapeCanvas(QAbstractScrollArea* parent, TreeCanvas* tc,
  	const std::multiset<ShapeI, CompareShapes>& sm);
  void highlightShape(VisualNode* node);

private:
  const QAbstractScrollArea* m_sa;
  VisualNode* m_targetNode = nullptr; // what is it?
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
  static constexpr int HEIGHT = 15;
  static constexpr int PIXELS_PER_VALUE = 5;
  static constexpr int SELECTION_WIDTH = 800;

  ShapeRect(int x, int y, int value, VisualNode*,
  	ShapeCanvas*, QGraphicsItem* parent = nullptr);
  VisualNode* getNode();
  // add to the scene
  void draw(QGraphicsScene* scene);
  QGraphicsRectItem visibleArea;
protected:
  void mousePressEvent (QGraphicsSceneMouseEvent*);
private:
  VisualNode* m_node;
  ShapeCanvas* m_canvas;
};

}}




#endif

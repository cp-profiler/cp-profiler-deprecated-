#ifndef SIMILAR_SHAPES_HH
#define SIMILAR_SHAPES_HH

#include <QDialog>
#include <set>

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
private:
  TreeCanvas& _tc;
public:
  explicit CompareShapes(TreeCanvas& tc);
  bool operator()(const ShapeI& s1, const ShapeI& s2) const;
};



class SimilarShapesWindow : public QDialog {
  Q_OBJECT

  friend class ::SimilarShapesCursor;
  friend class Filters;
  friend class ::TreeCanvas;

public:
  explicit SimilarShapesWindow(TreeCanvas* tc);
  ~SimilarShapesWindow(void);
  void drawHistogram(void);

  TreeCanvas* m_tc;
  ShapeCanvas* shapeCanvas;

public Q_SLOTS:
  void depthFilterChanged(int val);
  void countFilterChanged(int val);
private:

    /// Map of nodes for analyzing
  std::multiset<ShapeI, CompareShapes> shapesMap;

  QGraphicsScene histScene;
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
  VisualNode* m_targetNode; // what is it?
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
  void scroll(void);
};



class ShapeRect : public QGraphicsRectItem {
public:
  ShapeRect(qreal, qreal, qreal, qreal, VisualNode*,
  	SimilarShapesWindow*, QGraphicsItem* parent = nullptr);
  VisualNode* getNode(void);
  // add to the scene
  void draw(QGraphicsScene* scene);
  QGraphicsRectItem selectionArea;
protected:
  void mousePressEvent (QGraphicsSceneMouseEvent*);
private:
  VisualNode* _node;
  SimilarShapesWindow* _ssWindow;
};

}}




#endif

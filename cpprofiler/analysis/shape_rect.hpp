#include <QPen>
#include <QGraphicsScene>

namespace cpprofiler {
namespace analysis {


/// ******************************************
/// ************ SHAPE RECTANGLE *************
/// ******************************************

static const QColor transparent_red{255, 0, 0, 50};

class ShapeRect : public QGraphicsRectItem {

  VisualNode& m_node;
  SimilarShapesWindow* const m_ssWindow;

  void mousePressEvent(QGraphicsSceneMouseEvent*) override;

public:
  static constexpr int HEIGHT = 16;
  static constexpr int HALF_HEIGHT = HEIGHT / 2;
  static constexpr int PIXELS_PER_VALUE = 5;
  static constexpr int SELECTION_WIDTH = 600;

  ShapeRect(int x, int y, int width, VisualNode& n, SimilarShapesWindow*);

  void addToScene(QGraphicsScene* scene);
  QGraphicsRectItem visibleArea;

};

ShapeRect::ShapeRect(int x, int y, int width, VisualNode& node, SimilarShapesWindow* ssw)
    : QGraphicsRectItem(x, y - HALF_HEIGHT, SELECTION_WIDTH, HEIGHT, nullptr),
      visibleArea(x, y - HALF_HEIGHT + 1, width, HEIGHT - 2),
      m_node{node},
      m_ssWindow{ssw} {
  setBrush(Qt::white);
  QPen whitepen(Qt::white);
  setPen(whitepen);
  setFlag(QGraphicsItem::ItemIsSelectable);
  visibleArea.setBrush(transparent_red);
  visibleArea.setPen(whitepen);
}

void ShapeRect::addToScene(QGraphicsScene* scene) {
  scene->addItem(this);
  scene->addItem(&visibleArea);
}

void ShapeRect::mousePressEvent(QGraphicsSceneMouseEvent*) {
  m_ssWindow->highlightSubtrees(&m_node);
}

/// ******************************************
}}
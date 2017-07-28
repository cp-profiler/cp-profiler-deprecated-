

#include <QPen>
#include <QGraphicsScene>
#include "shape_rect.hh"
#include "histogram_win.hh"

namespace cpprofiler {
namespace analysis {

ShapeRect::ShapeRect(int x, int y, int width, HistogramWindow* ssw)
    : QGraphicsRectItem(x, y, SELECTION_WIDTH, HEIGHT, nullptr),
      m_ssWindow{ssw},
      visibleArea(x, y + 1, width, HEIGHT - 2) {
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
  m_ssWindow->handleRectClick(this);
}

}
}
#pragma once

#include <QGraphicsRectItem>

class VisualNode;

namespace cpprofiler {
namespace analysis {

class HistogramWindow;


/// ******************************************
/// ************ SHAPE RECTANGLE *************
/// ******************************************

static const QColor transparent_red{255, 0, 0, 50};

class ShapeRect : public QGraphicsRectItem {

  HistogramWindow* const m_ssWindow;

  void mousePressEvent(QGraphicsSceneMouseEvent*) override;

public:
  static constexpr int HEIGHT = 16;
  static constexpr int HALF_HEIGHT = HEIGHT / 2;
  static constexpr int PIXELS_PER_VALUE = 5;
  static constexpr int SELECTION_WIDTH = 600;

  ShapeRect(int x, int y, int width, HistogramWindow*);

  void addToScene(QGraphicsScene* scene);
  QGraphicsRectItem visibleArea;

};

/// ******************************************
}}
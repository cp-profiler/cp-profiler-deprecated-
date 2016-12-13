#pragma once

#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include "shape_rect.hh"
#include <vector>

class Shape;
class VisualNode;

namespace cpprofiler {
namespace analysis {


enum class SimilarityType {
  SHAPE, SUBTREE
};

using GroupsOfNodes_t = std::vector<std::vector<VisualNode*>>;


enum class ShapeProperty { SIZE, COUNT, HEIGHT };

enum class Align {
  CENTER,
  RIGHT
};


constexpr int NUMBER_WIDTH = 50;
constexpr int COLUMN_WIDTH = NUMBER_WIDTH + 10;
constexpr int ROW_HEIGHT = ShapeRect::HEIGHT + 1;

namespace detail {

static void addText(QGraphicsScene& scene, int col, int row,
                    QGraphicsSimpleTextItem* text_item,
                    Align alignment = Align::CENTER) {
  int item_width = text_item->boundingRect().width();
  int item_height = text_item->boundingRect().height();

  // center the item vertically at y
  int y_offset = item_height / 2;
  int x_offset = 0;
  
  switch (alignment) {
    case Align::CENTER:
      x_offset = (COLUMN_WIDTH - item_width) / 2; break;
    case Align::RIGHT:
      x_offset = COLUMN_WIDTH - item_width; break;
  }

  int x = col * COLUMN_WIDTH + x_offset;
  int y = row * ROW_HEIGHT   + y_offset;

  text_item->setPos(x, y - y_offset);
  scene.addItem(text_item);
}
};

static void addText(QGraphicsScene& scene, int col, int row, const char* text) {
  auto str_text_item = new QGraphicsSimpleTextItem{text};
  detail::addText(scene, col, row, str_text_item);
}

static void addText(QGraphicsScene& scene, int col, int row, int value) {
  auto int_text_item = new QGraphicsSimpleTextItem{QString::number(value)};
  detail::addText(scene, col, row, int_text_item, Align::RIGHT);
}


}
}
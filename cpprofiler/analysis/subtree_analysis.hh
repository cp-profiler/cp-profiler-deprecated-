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

struct ShapeInfo {
  int sol;
  int size;
  int height;
  std::vector<VisualNode*> nodes;
  Shape* s;
};

using GroupsOfNodes_t = std::vector<std::vector<VisualNode*>>;


enum class ShapeProperty { SIZE, COUNT, HEIGHT };


static ShapeProperty interpretShapeProperty(const QString& str) {
  if (str == "size") return ShapeProperty::SIZE;
  if (str == "count") return ShapeProperty::COUNT;
  if (str == "height") return ShapeProperty::HEIGHT;
  abort();
  return {};
}

/// Find a subtree from `vec` with the maximal `ShapeProperty` value and return that value
template<typename SI>
static int maxShapeValue(const std::vector<SI>& vec, ShapeProperty prop) {
  if (prop == ShapeProperty::SIZE) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.size < s2.size;
        });
    return res.size;
  } else if (prop == ShapeProperty::COUNT) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.get_count() < s2.get_count();
        });
    return res.get_count();
  } else if (prop == ShapeProperty::HEIGHT) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.height < s2.height;
        });
    return res.height;
  }

  return 1;
}

template<typename SI>
static int extractProperty(const SI& info, ShapeProperty prop) {

  int value;

  if (prop == ShapeProperty::SIZE) {
    value = info.size;
  } else if (prop == ShapeProperty::COUNT) {
    value = info.get_count();
  } else if (prop == ShapeProperty::HEIGHT) {
    value = info.height;
  } else {
    abort();
    value = -1;
  }

  return value;
}

enum class Align {
  CENTER,
  RIGHT
};

/// Sort elements of `vec` in place based on `prop`
template<typename SI>
static void sortSubtrees(std::vector<SI>& vec, ShapeProperty prop) {
  if (prop == ShapeProperty::SIZE) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.size > s2.size;
              });
  } else if (prop == ShapeProperty::COUNT) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.get_count() > s2.get_count();
              });
  } else if (prop == ShapeProperty::HEIGHT) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.height > s2.height;
              });
  }
}

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
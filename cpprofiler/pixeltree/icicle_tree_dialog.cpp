
/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "icicle_tree_dialog.hh"
#include <QAbstractScrollArea>
#include <QComboBox>
#include <climits>
#include <functional>

#include "treecanvas.hh"
#include "spacenode.hh"

using namespace cpprofiler::pixeltree;

/// TODO(maxim): be able to link rectangle to the a node
/// pos_x and pox_y -> node gid

IcicleTreeDialog::IcicleTreeDialog(TreeCanvas* tc) : QDialog(tc) {
  qDebug() << "tc in IcicleTree address: " << tc;

  this->resize(INIT_WIDTH, INIT_HEIGHT);
  this->setWindowTitle(
      QString::fromStdString(tc->getExecution()->getData()->getTitle()));

  scrollArea_ = new QAbstractScrollArea();

  auto layout = new QVBoxLayout();
  setLayout(layout);

  layout->addWidget(scrollArea_);

  auto optionsLayout = new QHBoxLayout();
  layout->addLayout(optionsLayout);

  QLabel* colorMapLabel = new QLabel("color map by");
  colorMapLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  auto color_map_cb = new QComboBox();
  optionsLayout->addWidget(colorMapLabel);
  optionsLayout->addWidget(color_map_cb);
  color_map_cb->addItem("default");
  color_map_cb->addItem("domain reduction");
  color_map_cb->addItem("node time");

  canvas_ = new IcicleTreeCanvas(scrollArea_, tc);

  setAttribute(Qt::WA_DeleteOnClose, true);

  connect(this, SIGNAL(windowResized()), canvas_, SLOT(resizeCanvas()));
  connect(color_map_cb, SIGNAL(currentTextChanged(const QString&)), canvas_,
          SLOT(changeColorMapping(const QString&)));
}

void IcicleTreeDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  emit windowResized();
}

IcicleTreeCanvas::IcicleTreeCanvas(QAbstractScrollArea* parent, TreeCanvas* tc)
    : QWidget(parent), sa_(*parent), tc_(*tc) {
  connect(sa_.horizontalScrollBar(), SIGNAL(valueChanged(int)), this,
          SLOT(sliderChanged(int)));

  /// TODO(maxim): find out the 'width' of the icicle tree

  setMouseTracking(true);

  icicle_image_.setPixelWidth(4);
  icicle_image_.setPixelHeight(8);
}

void IcicleTreeCanvas::paintEvent(QPaintEvent*) {
  if (icicle_image_.image() == nullptr) return;

  QPainter painter(this);

  painter.drawImage(QPoint{1, 1}, *icicle_image_.image());
}

void IcicleTreeCanvas::resizeCanvas() {
  auto sa_width = sa_.viewport()->width();
  auto sa_height = sa_.viewport()->height();
  icicle_image_.resize(sa_width, sa_height);
  this->resize(sa_width, sa_height);
  redrawAll();
}

void IcicleTreeCanvas::redrawAll() {
  icicle_image_.clear();

  drawIcicleTree();

  icicle_image_.update();
  /// added 10 of padding here
  sa_.horizontalScrollBar()->setRange(
      0, icicle_width - sa_.viewport()->width() + 10);
  sa_.horizontalScrollBar()->setPageStep(sa_.viewport()->width());
  sa_.horizontalScrollBar()->setSingleStep(100);  /// the value is arbitrary

  repaint();  /// TODO(maxim): do I need this?
}

void IcicleTreeCanvas::drawIcicleTree() {
  icicle_rects_.clear();

  auto& na = *tc_.get_na();

  auto& root = *na[0];

  x_global_ = 0;
  cur_depth_ = 0;

  domain_red_sum = 0;

  /// TODO(maxim): construct once, redraw many times
  auto extent = processNode(root);
  icicle_width = extent.second;

  qDebug() << "average domain reduction: "
           << domain_red_sum / tc_.get_stats().allNodes();
}

/// This assignes a color for every node on the icicle tree
static QRgb getColor(const SpaceNode& node) {
  QRgb color;
  switch (node.getStatus()) {
    case BRANCH: {
      color = qRgb(50, 50, 255);
      break;
    }
    case FAILED: {
      color = qRgb(255, 50, 50);
      break;
    }
    case SOLVED: {
      color = qRgb(50, 255, 50);
      break;
    }
    default: { color = qRgb(255, 255, 255); }
  }
  return color;
}

std::pair<int, int> IcicleTreeCanvas::processNode(SpaceNode& node) {
  // auto yoff = _sa->verticalScrollBar()->value();

  ++cur_depth_;

  const int kids = node.getNumberOfChildren();
  // qDebug() << "kids: " << kids;
  auto& na = *tc_.get_na();

  int x_begin = INT_MAX;

  int x_end = 0;

  for (int i = 0; i < kids; ++i) {
    auto& kid = *node.getChild(na, i);

    auto extent = processNode(kid);
    auto x1 = extent.first;
    auto x2 = extent.second;

    if (x1 < x_begin) x_begin = x1;
    if (x2 > x_end) x_end = x2;
  }

  if (kids == 0) {
    x_begin = x_global_;
    x_end = x_begin + 1;
    ++x_global_;
  }

  auto data = tc_.getExecution()->getData();

  auto gid = node.getIndex(na);
  auto* entry = data->getEntry(gid);

  // auto* entry = data->getEntry(node);
  /// entry to domain size (reduction)
  auto domain_red = entry == nullptr ? 0 : entry->domain;
  // qDebug() << "domain reduction: " << domain_red;
  /// NOTE(maxim): sometimes domain reduction is negative
  /// (domains appear to be larger in children nodes!?)
  domain_red_sum += domain_red;

  QRgb color;

  switch (color_mapping_type) {
    case ColorMappingType::DEFAULT: {
      color = getColor(node);
    } break;
    case ColorMappingType::DOMAIN_REDUCTION: {
      /// the smaller the value, the darker the color
      int color_value = 255 * static_cast<float>(domain_red);
      if (color_value < 0) color_value = 0;
      color = QColor::fromHsv(0, 0, color_value).rgba();
    } break;
    case ColorMappingType::NODE_TIME: {
      auto node_time = entry == nullptr ? 0 : entry->node_time;
      /// TODO(maxim): need to normalize the node time
      int color_value = static_cast<float>(node_time);
      color = QColor::fromHsv(0, 0, color_value).rgba();
    }
  }

  auto xoff = sa_.horizontalScrollBar()->value();

  /// Actually drawing the rectangle
  int rect_x = x_begin - xoff;
  int rect_y = cur_depth_;
  int rect_width = x_end - x_begin;
  int rect_height = icicle_image_.pixel_height();

  icicle_image_.drawRect(rect_x, rect_width, rect_y, color);
  icicle_rects_.push_back(
      IcicleRect{rect_x, rect_y, rect_width, rect_height, node});

  --cur_depth_;

  return std::make_pair(x_begin, x_end);
}

void IcicleTreeCanvas::sliderChanged(int) {
  /// calls redrawAll not more often than 60hz
  // TODO(maxim): this goes into infinite loop somehow...
  // maybeCaller.call([this]() { redrawAll(); });
}

SpaceNode* IcicleTreeCanvas::getNodeByXY(int x, int y) const {
  // Find rectangle by x and y (binary or linear search)
  for (int i = 0; i < icicle_rects_.size(); i++) {
    auto& rect = icicle_rects_[i];

    if ((rect.x <= x) && (x <= rect.x + rect.width) && (rect.y <= y) &&
        (y <= rect.y + rect.height)) {
      return &rect.node;
    }
  }

  return nullptr;
}

static void unselectNodes(std::vector<SpaceNode*>& nodes_selected) {
  for (auto node : nodes_selected) {
    static_cast<VisualNode*>(node)->setHovered(false);
  }
  nodes_selected.clear();
}

void IcicleTreeCanvas::mouseMoveEvent(QMouseEvent* event) {
  const auto x = event->x() / icicle_image_.pixel_width();
  const auto y = event->y() / icicle_image_.pixel_height();

  auto* node = getNodeByXY(x, y);

  if (node == nullptr) return;

  /// Do stuff not more often than 60hz
  maybeCaller.call([this, node]() {
    unselectNodes(nodes_selected);
    nodes_selected.push_back(node);
    static_cast<VisualNode*>(node)->setHovered(true);
    tc_.update();
  });
}

void IcicleTreeCanvas::mousePressEvent(QMouseEvent* event) {
  /// do nothing for now
}

void IcicleTreeCanvas::changeColorMapping(const QString& text) {
  if (text == "default") {
    qDebug() << "to default color mapping";
    color_mapping_type = ColorMappingType::DEFAULT;
  } else if (text == "domain reduction") {
    qDebug() << "to domain reduction color mapping";
    color_mapping_type = ColorMappingType::DOMAIN_REDUCTION;
  } else if (text == "node time") {
    color_mapping_type = ColorMappingType::NODE_TIME;
  }

  redrawAll();
}
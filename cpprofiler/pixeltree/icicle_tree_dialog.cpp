
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
#include "data.hh"
#include "libs/perf_helper.hh"

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
  auto& na = tc_.getExecution()->getNA();
  leafCount.resize(na.size());
  initTreeStatistic(*na[0], 0);
  connect(sa_.horizontalScrollBar(), SIGNAL(valueChanged(int)), this,
          SLOT(sliderChanged(int)));
  connect(sa_.verticalScrollBar(), SIGNAL(valueChanged(int)), this,
          SLOT(sliderChanged(int)));
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
  perfHelper.begin("icicle tree: update");
  icicle_image_.update();
  perfHelper.end();
  /// added 10 of padding here
  int icicle_width = leafCount[0];
  sa_.horizontalScrollBar()->setRange(
      0, icicle_width - sa_.viewport()->width() + 10);
  sa_.horizontalScrollBar()->setPageStep(sa_.viewport()->width());
  sa_.horizontalScrollBar()->setSingleStep(100);  /// the value is arbitrary

  repaint();  /// TODO(maxim): do I need this?
}

void IcicleTreeCanvas::drawIcicleTree() {
  icicle_rects_.clear();
  auto& na = tc_.getExecution()->getNA();
  auto& root = *na[0];
  int idx = 0, curx = 0, cury = 0;
  int yoff = 0, xoff = sa_.horizontalScrollBar()->value();
  int width = sa_.viewport()->width();
  int depth = sa_.viewport()->height();
  qDebug() << "curx: " << curx << ", xoff: " << xoff
        << ", width: " << width << ", yoff: " << yoff << ", depth: " << depth;
  perfHelper.begin("icicle tree: dfs");
  dfsVisible(root, idx, curx, cury, xoff, width, yoff, depth);
  perfHelper.end();

  perfHelper.begin("icicle tree: draw");
  drawRects();
  perfHelper.end();
  qDebug() << "average domain reduction: "
           << domain_red_sum / tc_.get_stats().allNodes();
}

void IcicleTreeCanvas::drawRects() {
  domain_red_sum = 0;
  for (auto rect: icicle_rects_) {
    SpaceNode& node = rect.node;
    QRgb color = getColorByType(node);
    icicle_image_.drawRect(rect.x, rect.width, rect.y, color);
  }
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

QRgb IcicleTreeCanvas::getColorByType(const SpaceNode& node) {
  QRgb color;
  auto& na = tc_.getExecution()->getNA();
  auto data = tc_.getExecution()->getData();
  auto gid = node.getIndex(na);
  auto* entry = data->getEntry(gid);
  auto domain_red = entry == nullptr ? 0 : entry->domain;
  domain_red_sum += domain_red;
  switch (IcicleTreeCanvas::color_mapping_type) {
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
  return color;
}

void IcicleTreeCanvas::dfsVisible(SpaceNode& root, int idx, int curx, int cury,
  int xoff, int width, int yoff, int depth) {
  if (cury > depth) return;

  const int kids = root.getNumberOfChildren();
  auto& na = tc_.getExecution()->getNA();
  int nextxL = curx, nextxR;
  for (int i = 0; i < kids; i++) {
    if (nextxL > xoff + width) break;
    int kidIdx = root.getChild(i);
    SpaceNode& kid = *na[kidIdx];
    nextxR = nextxL + leafCount[kidIdx];
    if (nextxR >= xoff)
      dfsVisible(kid, kidIdx, nextxL, cury+1, xoff, width, yoff, depth);
    nextxL = nextxR;
  }
  if (cury >= yoff && cury <= yoff + depth) {
    int rect_x = std::max(curx - xoff, 0);
    int rect_y = cury;
    int rect_width = std::min(xoff + width, curx + leafCount[idx]) - xoff - rect_x;
    int rect_height = icicle_image_.pixel_height();
    icicle_rects_.push_back(IcicleRect{rect_x, rect_y, rect_width, rect_height, root});
  }
}

void IcicleTreeCanvas::sliderChanged(int) {
  /// calls redrawAll not more often than 60hz
  maybeCaller.call([this]() { redrawAll(); });
}

SpaceNode* IcicleTreeCanvas::getNodeByXY(int x, int y) const {
  // Find rectangle by x and y (binary or linear search)
  for (uint i = 0; i < icicle_rects_.size(); i++) {
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

void IcicleTreeCanvas::mousePressEvent(QMouseEvent*) {
  /// do nothing for now
}

int IcicleTreeCanvas::initTreeStatistic(SpaceNode& root, int idx) {
  const int kids = root.getNumberOfChildren();
  auto& na = tc_.getExecution()->getNA();
  int cntRoot = kids?0: 1;
  for (int i=0; i < kids; i++) {
    SpaceNode& kid = *root.getChild(na, i);
    int kidIdx = root.getChild(i);
    int cntKid = initTreeStatistic(kid, kidIdx);
    cntRoot += cntKid;
  }
  leafCount[idx] = cntRoot;
  return cntRoot;
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

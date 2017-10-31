
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
#include <set>

#include "treecanvas.hh"
#include "execution.hh"
#include "nodetree.hh"
#include "globalhelper.hh"
#include "spacenode.hh"
#include "data.hh"
#include "libs/perf_helper.hh"

using namespace cpprofiler::pixeltree;

/// TODO(maxim): be able to link rectangle to the a node
/// pos_x and pox_y -> node gid

IcicleTreeDialog::IcicleTreeDialog(TreeCanvas* tc) : QDialog(tc) {

  this->resize(INIT_WIDTH, INIT_HEIGHT);
  this->setWindowTitle(
      QString::fromStdString(tc->getExecution().getTitle()));

  auto layout = new QVBoxLayout();
  setLayout(layout);

  scrollArea_ = new QAbstractScrollArea();
  layout->addWidget(scrollArea_);

  canvas_ = new IcicleTreeCanvas(scrollArea_, tc);
  connect(canvas_, &IcicleTreeCanvas::canvasAffected, [tc]() {
    tc->updateCanvas(false);
  });

  connect(tc, &TreeCanvas::nodeSelected, canvas_, &IcicleTreeCanvas::selectNode);

  auto optionsLayout = new QHBoxLayout();
  layout->addLayout(optionsLayout);

  {
    auto colorMapLabel = new QLabel("color map by");
    colorMapLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    optionsLayout->addWidget(colorMapLabel);
  }

  {
    auto color_map_cb = new QComboBox();
    color_map_cb->addItem("default");
    color_map_cb->addItem("domain reduction");
    color_map_cb->addItem("node time");
    color_map_cb->addItem("variables");
    optionsLayout->addWidget(color_map_cb);
    connect(color_map_cb, SIGNAL(currentTextChanged(const QString&)), canvas_,
    SLOT(changeColorMapping(const QString&)));
  }

  setAttribute(Qt::WA_DeleteOnClose, true);

  auto controlLayout = new QHBoxLayout();
  layout->addLayout(controlLayout);

  {
    auto pSizeLabel = new QLabel("pixel size");
    pSizeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    controlLayout->addWidget(pSizeLabel);
  }

  {
    auto pixelSizeSB = new QSpinBox(this);
    pixelSizeSB->setRange(1, 40);
    pixelSizeSB->setValue(PixelImage::DEFAULT_PIXEL_SIZE);
    controlLayout->addWidget(pixelSizeSB);
    connect(pixelSizeSB, SIGNAL(valueChanged(int)), canvas_,
            SLOT(resizePixel(int)));
  }

  {
    auto compressLevelLabel = new QLabel("compression");
    compressLevelLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    controlLayout->addWidget(compressLevelLabel);
  }

  {
    auto compressLevelSB = new QSpinBox(this);
    compressLevelSB->setRange(0, canvas_->getTreeHeight());
    controlLayout->addWidget(compressLevelSB);
    connect(compressLevelSB, SIGNAL(valueChanged(int)), canvas_,
            SLOT(compressLevelChanged(int)));
  }

  connect(this, SIGNAL(windowResized()), canvas_, SLOT(resizeCanvas()));
}

void IcicleTreeDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  emit windowResized();
}

VisualNode* IcicleTreeCanvas::findLeftLeaf() {
  VisualNode* res = nullptr;
  int maxD = -1;
  for (IcicleRect rect: icicle_rects_) {

    if (rect.x == 0 && rect.y > maxD) {
      int idx = node_tree.getIndex(&rect.node);
      if (statistic[idx].height >= compressLevel) {
        maxD = rect.y;
        res = &rect.node;
      }
    }

  }
  return res;
}

void IcicleTreeCanvas::compressLevelChanged(int value) {
  auto& na = node_tree.getNA();
  //int leafIndex;
  compressLevel = value;
  compressInit(*na[0], 0, 0);
  VisualNode* lftLeaf = findLeftLeaf();
  //leafIndex = lftLeaf->getIndex(na);
  int xoff = statistic[lftLeaf->getIndex(na)].absX * icicle_image_.pixel_height();
  sa_.horizontalScrollBar()->setValue(xoff);
  redrawAll();
  //leafIndex = findLeftLeaf()->getIndex(na);
  sa_.horizontalScrollBar()->setValue(xoff);
}

bool IcicleTreeCanvas::compressInit(VisualNode& root, int idx, int absX) {
  const int kids = root.getNumberOfChildren();
  const auto& na = node_tree.getNA();
  bool hasSolved = false;

  int leafCnt = 0, expectSolvedCnt = 0, actualSolvedCnt = 0;
  statistic[idx].ns = root.getStatus();
  statistic[idx].absX = absX;
  for (int i = 0; i < kids; i++) {
    int kidIdx = root.getChild(i);
    VisualNode& kid = *na[kidIdx];
    if (kid.hasSolvedChildren()) {
      expectSolvedCnt++;
    }
    if (statistic[kidIdx].height >= compressLevel) {
      bool kidRes = compressInit(kid, kidIdx, absX + leafCnt);
      hasSolved |= kidRes;
      leafCnt += statistic[kidIdx].leafCnt;
      if (kidRes) actualSolvedCnt++;
    }
  }
  statistic[idx].leafCnt = leafCnt? leafCnt: 1;
  if (kids && statistic[idx].height == compressLevel)
    statistic[idx].ns = root.hasSolvedChildren()? SOLVED: FAILED;
  else if (expectSolvedCnt > actualSolvedCnt)
    statistic[idx].ns = SOLVED;
  return hasSolved | (statistic[idx].ns == SOLVED);
}

static std::map<std::string, QRgb> initVariableMap(const TreeCanvas& tc, const NodeTree& nt) {

  using std::string; using std::set; using std::map; using std::vector;

  map<string, QRgb> var2color;

  auto& na = nt.getNA();

  set<string> all_vars_set;

  for (auto gid = 0; gid < na.size(); gid++) {
    auto label = tc.getLabel(gid);

    string var = "";
    auto found = findAnyOf(label, "=", "!=", "<", ">", ">=", "=<");
    if (found != string::npos) var = label.substr(0, found);
    all_vars_set.insert(var);
  }

  auto all_vars = vector<string>{all_vars_set.begin(), all_vars_set.end()};
  std::sort(all_vars.begin(), all_vars.end());

  for (auto var_idx = 0u; var_idx < all_vars.size(); var_idx++) {
    auto var = all_vars[var_idx];
    auto val = ceil(var_idx * 255 / all_vars.size());
    auto color = QColor::fromHsv(val, 200, 255).rgba();
    var2color[var] = color;
  }

  return var2color;
}

IcicleTreeCanvas::IcicleTreeCanvas(QAbstractScrollArea* parent, TreeCanvas* tc)
    : QWidget(parent), sa_(*parent), tc_(*tc), node_tree(tc->getExecution().nodeTree()) {
  compressLevel = 0;
  auto& na = node_tree.getNA();
  statistic.resize(na.size());
  initTreeStatistic(*node_tree.getRoot(), 0, 0);
  connect(sa_.horizontalScrollBar(), SIGNAL(valueChanged(int)), this,
          SLOT(sliderChanged(int)));
  connect(sa_.verticalScrollBar(), SIGNAL(valueChanged(int)), this,
          SLOT(sliderChanged(int)));
  setMouseTracking(true);

  var2color = initVariableMap(tc_, node_tree);
}

void IcicleTreeCanvas::resizePixel(int value) {
  int absXL = sa_.horizontalScrollBar()->value() / icicle_image_.pixel_height();
  int newXoff = absXL * value;
  icicle_image_.setPixelWidth(value);
  icicle_image_.setPixelHeight(value);
  sa_.horizontalScrollBar()->setValue(newXoff);
  redrawAll();
  // fix scroll bar(scroll bar will change automatically when the bar touch right border)
  sa_.horizontalScrollBar()->setValue(newXoff);
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
  maybeCaller.call([this]() { redrawAll(); });
}

void IcicleTreeCanvas::redrawAll() {
  icicle_image_.clear();
  drawIcicleTree();
  // perfHelper.begin("icicle tree: update");
  icicle_image_.update();
  // perfHelper.end();
  /// added 10 of padding here
  int icicle_width = statistic[0].leafCnt * icicle_image_.pixel_height();
  sa_.horizontalScrollBar()->setRange(
      0, icicle_width - sa_.viewport()->width() + 10);
  sa_.horizontalScrollBar()->setPageStep(sa_.viewport()->width());
  sa_.horizontalScrollBar()->setSingleStep(100);  /// the value is arbitrary

  repaint();  /// TODO(maxim): do I need this?
}

void IcicleTreeCanvas::drawIcicleTree() {
  icicle_rects_.clear();
  auto& na = node_tree.getNA();
  auto& root = *node_tree.getRoot();
  int idx = 0, curx = 0, cury = 0;
  int yoff = 0 / icicle_image_.pixel_height();
  int xoff = sa_.horizontalScrollBar()->value() / icicle_image_.pixel_height();
  int width = sa_.viewport()->width();
  int depth = sa_.viewport()->height();
  // perfHelper.begin("icicle tree: dfs");
  dfsVisible(root, idx, curx, cury, xoff, width, yoff, depth);
  // perfHelper.end();

  // perfHelper.begin("icicle tree: draw");
  drawRects();
  // perfHelper.end();
}

void IcicleTreeCanvas::drawRects() {
  domain_red_sum = 0;
  for (auto rect: icicle_rects_) {
    QRgb color = getColorByType(rect.node);
    icicle_image_.drawRect(rect.x, rect.width, rect.y, color);
  }
}

/// This assignes a color for every node on the icicle tree
static QRgb getColor(NodeStatus ns) {
  QRgb color;
  switch (ns) {
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

QRgb IcicleTreeCanvas::getColorByType(const VisualNode& node) {

  if (selectedNode == &node) { return QColor::fromHsv(0, 150, 150).rgba();}

  QRgb color;
  auto& na = node_tree.getNA();
  auto& data = tc_.getExecution().getData();
  auto gid = node.getIndex(na);
  auto* entry = data.getEntry(gid);
  // auto domain_red = entry == nullptr ? 0 : entry->domain;
  auto domain_red = 0;
  domain_red_sum += domain_red;
  switch (IcicleTreeCanvas::color_mapping_type) {
    case ColorMappingType::DEFAULT: {
      NodeStatus ns = statistic[gid].ns;
      color = getColor(ns);
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
    case ColorMappingType::VARIABLES: {
      auto gid = node_tree.getIndex(&node);
      auto label = tc_.getLabel(gid);

      std::string var = "";
      auto found = findAnyOf(label, "=", "!=", "<", ">", ">=", "=<");
      if (found != std::string::npos) var = label.substr(0, found);

      color = var2color[var];
    } break;
  }

  return color;
}

void IcicleTreeCanvas::dfsVisible(VisualNode& root, int idx, int curx, int cury,
  const int xoff, const int width, const int yoff, const int depth) {
  if (cury > depth) return;

  const int kids = root.getNumberOfChildren();
  auto& na = node_tree.getNA();
  int nextxL = curx;
  int nextxR;

  for (int i = 0; i < kids; i++) {
    if (nextxL > xoff + width) break;
    int kidIdx = root.getChild(i);
    if (statistic[kidIdx].height >= compressLevel) {
      VisualNode& kid = *na[kidIdx];
      nextxR = nextxL + statistic[kidIdx].leafCnt;
      if (nextxR >= xoff)
        dfsVisible(kid, kidIdx, nextxL, cury+1, xoff, width, yoff, depth);
      nextxL = nextxR;
    }
  }

  if (cury >= yoff && cury <= yoff + depth) {
    int rectAbsXL = std::max(curx, xoff);
    int rectAbsXR = std::min(curx + statistic[idx].leafCnt, xoff + width);
    int rectAbsY = cury;
    int height = icicle_image_.pixel_height();
    int x = rectAbsXL - xoff;
    int y = rectAbsY - yoff;
    int width = rectAbsXR - rectAbsXL;
    icicle_rects_.push_back(IcicleRect{x, y, width, height, root});
  }
}

void IcicleTreeCanvas::sliderChanged(int) {
  /// calls redrawAll not more often than 60hz
  maybeCaller.call([this]() { redrawAll(); });
}

VisualNode* IcicleTreeCanvas::getNodeByXY(int x, int y) const {
  // Find rectangle by x and y (binary or linear search)
  for (auto& rect : icicle_rects_) {
    if ((rect.x <= x) && (x < rect.x + rect.width) &&
        (rect.y == y)) {

      return &rect.node;
    }
  }
  return nullptr;
}

static void unselectNodes(std::vector<VisualNode*>& nodes_selected) {
  for (auto node : nodes_selected) {
    node->setHovered(false);
  }
  nodes_selected.clear();
}

void IcicleTreeCanvas::selectNode(VisualNode* node) {
  if (node == nullptr) return;

  unselectNodes(nodes_selected);
  selectedNode = node;
  nodes_selected.push_back(node);
  redrawAll();
}

void IcicleTreeCanvas::mouseMoveEvent(QMouseEvent* event) {
  const auto x = event->x() / icicle_image_.pixel_width();
  const auto y = event->y() / icicle_image_.pixel_height();

  auto* node = getNodeByXY(x, y);

  if (node == nullptr && nodes_selected.size() > 0) {
    selectedNode = nullptr;
    unselectNodes(nodes_selected);
    redrawAll();
    emit canvasAffected();
  }

  if (node == nullptr) return;

  if (node != selectedNode) {
    selectedNode = node;

    /// Do stuff not more often than 60hz
    maybeCaller.call([this, node]() {
      unselectNodes(nodes_selected);
      nodes_selected.push_back(node);
      node->setHovered(true);
      emit canvasAffected();
      redrawAll();
    });

  }

  

}

void IcicleTreeCanvas::mousePressEvent(QMouseEvent*) {

  if (selectedNode) {
    auto gid = node_tree.getIndex(selectedNode);
    tc_.navigateToNodeById(gid);
  }
}

IcicleNodeStatistic IcicleTreeCanvas::initTreeStatistic(VisualNode& root, int idx, int absX) {
  const int kids = root.getNumberOfChildren();
  auto& na = node_tree.getNA();
  IcicleNodeStatistic cntRoot = IcicleNodeStatistic{kids?0: 1, 0, absX, root.getStatus()};
  for (int i=0; i < kids; i++) {
    VisualNode& kid = *root.getChild(na, i);
    int kidIdx = root.getChild(i);
    IcicleNodeStatistic cntKid = initTreeStatistic(kid, kidIdx, absX + cntRoot.leafCnt);
    cntRoot.leafCnt += cntKid.leafCnt;
    cntRoot.height = std::max(cntRoot.height, cntKid.height + 1);
  }
  statistic[idx] = cntRoot;
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
  } else if (text == "variables") {
    color_mapping_type = ColorMappingType::VARIABLES;
  }

  redrawAll();
}

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
 *
 */

#include "pixelview.hh"
#include <chrono>
#include <cmath>
#include <algorithm> 
#include <stack>

using namespace std::chrono;
using std::vector; using std::list;

/// ******* PIXEL_TREE_DIALOG ********

PixelTreeDialog::PixelTreeDialog(TreeCanvas* tc)
  : QDialog(tc), canvas(&scrollArea, *tc)
{

  this->resize(600, 400);

  /// set Title
  this->setWindowTitle(QString::fromStdString(tc->getData()->getTitle()));

  QVBoxLayout* layout = new QVBoxLayout();
  QHBoxLayout* controlLayout = new QHBoxLayout();
  setLayout(layout);
  layout->addWidget(&scrollArea);
  layout->addLayout(controlLayout);

  /// ***** control panel *****
  QPushButton* scaleUp = new QPushButton(this);
  scaleUp->setText("+");

  QPushButton* scaleDown = new QPushButton(this);
  scaleDown->setText("-");

  QLabel* compLabel = new QLabel("compression");
  compLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QSpinBox* compressionSB = new QSpinBox(this);
  compressionSB->setRange(1, 10000);

  controlLayout->addWidget(scaleDown);
  controlLayout->addWidget(scaleUp);
  controlLayout->addWidget(compLabel);
  controlLayout->addWidget(compressionSB);

  /// *************************

  // canvas = new PixelTreeCanvas(&scrollArea, tc);

  connect(scaleDown, SIGNAL(clicked()), &canvas, SLOT(scaleDown()));
  connect(scaleUp, SIGNAL(clicked()), &canvas, SLOT(scaleUp()));
  connect(compressionSB, SIGNAL(valueChanged(int)),
    &canvas, SLOT(compressionChanged(int)));

  connect(this, SIGNAL(windowResized()), &canvas, SLOT(resizeCanvas()));

  setAttribute(Qt::WA_QuitOnClose, true);
  setAttribute(Qt::WA_DeleteOnClose, true);

}

PixelTreeDialog::~PixelTreeDialog(void) {
  // delete canvas;
}

void
PixelTreeDialog::resizeEvent(QResizeEvent * re) {
  emit windowResized();
}


PixelTreeCanvas::~PixelTreeCanvas(void) {

}

/// ***********************************


/// ******** PIXEL_TREE_CANVAS ********

PixelTreeCanvas::PixelTreeCanvas(QWidget* parent, TreeCanvas& tc)
  : QWidget(parent), _tc(tc), _data(*tc.getData()), _na(tc.na), depthAnalysis(tc)
{

  _sa = static_cast<QAbstractScrollArea*>(parentWidget());
  _vScrollBar = _sa->verticalScrollBar();

  _nodeCount = tc.stats.solutions + tc.stats.failures
                       + tc.stats.choices + tc.stats.undetermined;

  max_depth = tc.stats.maxDepth;

  // if (_tc->getData()->isRestarts()) {
  //   max_depth++; /// consider the first, dummy node
  //   _nodeCount++;
  // }

  /// scrolling business
  _sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  _sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  // _sa->setAutoFillBackground(true);

  connect (_sa->horizontalScrollBar(), SIGNAL(valueChanged (int)), this, SLOT(sliderChanged(int)));
  connect (_sa->verticalScrollBar(), SIGNAL(valueChanged (int)), this, SLOT(sliderChanged(int)));

  da_data = depthAnalysis.runMSL();
  
  constructPixelTree();
  compressPixelTree(1);
  compressDepthAnalysis(da_data_compressed, 1);
  compressTimeHistogram(time_arr, 1);
  compressDomainHistogram(domain_arr, 1);
  gatherVarData();
  compressVarData(var_decisions_compressed, 1);

  redrawAll();

}

void
PixelTreeCanvas::paintEvent(QPaintEvent*) {
  QPainter painter(this);

  /// start at (1;1) to prevent strage artifact
  painter.drawImage(1, 1, pixel_image.image(), 0, 0, _sa->viewport()->width(), _sa->viewport()->height());
}

void
PixelTreeCanvas::constructPixelTree(void) {

  high_resolution_clock::time_point time_begin = high_resolution_clock::now();

  /// how many of each values after compression
  vlines = ceil((float)_nodeCount / approx_size);

  domain_red_arr.clear(); domain_red_arr.resize(vlines);


  /// get a root
  VisualNode* root = (*_na)[0];

  vline_idx    = 0;
  node_idx     = 0;

  alpha_factor = 100.0 / approx_size;

  // pixel_data = traverseTree(root);
  pixel_data = traverseTreePostOrder(root);

  high_resolution_clock::time_point time_end = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(time_end - time_begin);
  std::cout << "Pixel Tree construction took: " << time_span.count() << " seconds." << std::endl;

}

void
PixelTreeCanvas::compressPixelTree(int compression) {
  /// take pixel_data and create vlineData

  int vlines = ceil(static_cast<float>(pixel_data.pixel_list.size()) / compression);

  auto& compressed_list = pixel_data.compressed_list;

  compressed_list = vector<list<PixelItem*>>(vlines, list<PixelItem*>());

  for (unsigned int pixel_id = 0; pixel_id < pixel_data.pixel_list.size(); pixel_id++) {
    unsigned int vline_id = pixel_id / compression;
    compressed_list[vline_id].push_back(&pixel_data.pixel_list[pixel_id]);
  }
}

void
PixelTreeCanvas::compressDepthAnalysis
(std::vector< std::vector<unsigned int> >& da_data_compressed, int compression) {

  auto data_length = da_data.at(0).size();
  auto vlines = ceil((float)data_length / compression);

  // da_data_compressed = vector<vector<unsigned int> >(max_depth, vector<unsigned int>(vlines));
  da_data_compressed.clear();
  da_data_compressed.resize(max_depth);
  for (auto& v : da_data_compressed) {
    v.resize(vlines);
  }

  /// for every depth level
  for (auto depth = 0; depth < da_data.size(); depth++) {

    auto group_count = 0;
    auto group_value = 0;

    for (auto i = 0; i < data_length; i++) {

      group_count++;
      group_value += da_data[depth][i];

      if (group_count == compression) {
        unsigned int vline_id = i / compression;
        da_data_compressed[depth][vline_id] = group_value / group_count;
        group_count = 0; group_value = 0;
      }

    }

    /// deal with the last (not full) group
    if (group_count > 0) {
      auto vline_id = ceil((float)data_length / compression) - 1;
      da_data_compressed[depth][vline_id] = group_value / group_count;
    }
  }
}

void
PixelTreeCanvas::compressTimeHistogram(vector<float>& compressed, int compression) {

  auto pixel_list = pixel_data.pixel_list;
  auto data_length = pixel_list.size();
  auto vlines = ceil(data_length / compression);

  compressed.clear();
  compressed.resize(vlines);

  auto group_count = 0;
  auto group_value = 0.0f;

  for (auto i = 0; i < pixel_list.size(); i++) {
    group_count++;

    /// TODO: ignore UNDET nodes (crashes otherwise)
    auto value = _data.getEntry(pixel_list[i].node()->getIndex(*_na))->node_time;
    group_value += value;

    if (group_count == compression) {
      unsigned int vline_id = i / compression;
      compressed[vline_id] = group_value / group_count;
      group_count = 0; group_value = 0;
    }
  }

  /// deal with the last (not full) group
  if (group_count > 0) {
    unsigned int vline_id = data_length / compression;
    compressed[vline_id] = group_value / group_count;
  }

}


/// TODO: try to avoid code duplication
void
PixelTreeCanvas::compressDomainHistogram(vector<float>& compressed, int compression) {

  auto pixel_list = pixel_data.pixel_list;
  auto data_length = pixel_list.size();
  auto vlines = ceil(data_length / compression);

  if (compressed.size() > 0) compressed.clear();
  compressed.resize(vlines);

  auto group_count = 0;
  auto group_value = 0.0f;

  for (auto i = 0; i < pixel_list.size(); i++) {
    group_count++;

    auto value = _data.getEntry(pixel_list[i].node()->getIndex(*_na))->domain;
    group_value += value;

    if (group_count == compression) {
      unsigned int vline_id = i / compression;
      compressed[vline_id] = group_value / group_count;
      group_count = 0; group_value = 0;
    }
  }

  /// deal with the last (not full) group
  if (group_count > 0) {
    unsigned int vline_id = data_length / compression;
    compressed[vline_id] = group_value / group_count;
  }
}

void
PixelTreeCanvas::compressVarData(vector<vector<int> >& compressed, int compression) {
  auto data_length = var_decisions.size();
  auto vlines = ceil(data_length / compression);

  if (compressed.size() > 0) compressed.clear();
  compressed.resize(vlines);

  auto group_count = 0;
  auto vline = 0;

  compressed.at(0).resize(compression);

  for (auto i = 0; i < var_decisions.size(); i++) {
    group_count++;

    compressed.at(vline).at(group_count - 1) = var_decisions[i];

    if (group_count == compression) {
      vline++;
      if (vline >= vlines) return;
      compressed.at(vline).resize(compression);
      group_count = 0;
    }
  }

}

void
PixelTreeCanvas::gatherVarData() {

  auto data_length = pixel_data.pixel_list.size();
  var_decisions.reserve(data_length);

  for (auto& pixel : pixel_data.pixel_list) {
    auto label = _data.getLabel(pixel.node()->getIndex(*_na)); /// 1. get label
    // qDebug() << "label: " << label.c_str();

    if (label == "") { qDebug() << "empty label"; }

    /// 2. get variable name
    auto found = label.find(' ');

    string var = "";
    if (found!=std::string::npos)
      var = label.substr(0, found);
    // qDebug() << "var: " << var.c_str();

    /// 3. check if we already know the variable

    auto var_id = -1;

    for (auto i = 0; i < vars.size(); i++) {
      if (vars[i] == var) {
        var_id = i;
        break;
      }
    }

    if (var_id == -1) { /// no such variable 
      qDebug() << "no such variable (" << var.c_str() << ")";
      vars.push_back(var);
      var_id = vars.size() - 1;
    } else {
      qDebug() << "variable found (" << var.c_str() << ")";
    }

    /// rememeber decision variables
    var_decisions.push_back(var_id);
  }
}

void
PixelTreeCanvas::compressVarData() {

}

PixelData
PixelTreeCanvas::traverseTree(VisualNode* root) {

  /// 0. prepare a stack for exploration
  std::stack<VisualNode*> explorationStack;
  std::stack<unsigned int> depthStack;

  PixelData pixelData(_nodeCount);

  /// 1. push the root node
  explorationStack.push(root);
  depthStack.push(1);

  /// 2. traverse the stack
  while(explorationStack.size() > 0) {

    VisualNode* node   = explorationStack.top(); explorationStack.pop();
    unsigned int depth = depthStack.top();       depthStack.pop();

    {
      DbEntry* entry = _data.getEntry(node->getIndex(*_na));
      /// TODO: find out if I need node_idx here
      pixelData.pixel_list.emplace_back(PixelItem(0, node, depth));

    }

    /// 2.1. add the children to the stack
    int kids = node->getNumberOfChildren();
    for (int i = kids - 1; i >= 0; --i) {
      auto kid = node->getChild(*_na, i);
      explorationStack.push(kid);
      depthStack.push(depth + 1);
    }

  }

  return pixelData;
}

PixelData
PixelTreeCanvas::traverseTreePostOrder(VisualNode* root) {

  std::stack<VisualNode*> nodeStack1;
  std::stack<unsigned int> depthStack1;

  std::stack<VisualNode*> nodeStack2;
  std::stack<unsigned int> depthStack2;

  PixelData pixelData(_nodeCount);

  nodeStack1.push(root);
  depthStack1.push(1);

  while (nodeStack1.size() > 0) {

    VisualNode* node = nodeStack1.top(); nodeStack1.pop();
    unsigned int depth = depthStack1.top(); depthStack1.pop();

    nodeStack2.push(node);
    depthStack2.push(depth);

    uint kids = node->getNumberOfChildren();
    for (uint i = 0; i < kids; ++i) {
      auto kid = node->getChild(*_na, i);
      nodeStack1.push(kid);
      depthStack1.push(depth + 1);
    }
  }

  while (nodeStack2.size() > 0) {

    VisualNode* node = nodeStack2.top(); nodeStack2.pop();
    unsigned int depth = depthStack2.top(); depthStack2.pop();

    {
      DbEntry* entry = _data.getEntry(node->getIndex(*_na));
      /// TODO: find out if I need node_idx here
      pixelData.pixel_list.emplace_back(PixelItem(0, node, depth));

    }
  }

  return pixelData;
}

void
PixelTreeCanvas::drawPixelTree(const PixelData& pixel_data) {

  auto xoff = _sa->horizontalScrollBar()->value(); // values should be in scaled pixels
  auto yoff = _sa->verticalScrollBar()->value();

  auto img_width = pixel_image.image().width();
  auto img_height = pixel_image.image().height();

  const auto& vline_data = pixel_data.compressed_list;

  /// Draw vertical lines for solutions

  for (auto vline = xoff; vline < vline_data.size(); vline++) {

    auto x = vline - xoff;
    if (x > img_width) break; /// out of image boundary

    /// Draw green vertical line for solutions
    for (const auto& pixel_item : vline_data[vline]) {
      if (pixel_item->node()->getStatus() == SOLVED) {
        for (unsigned y = 0; y < max_depth - yoff; y++) {
          pixel_image.drawPixel(x, y, qRgb(0, 255, 0));
        }
      }
    }


    auto isSelected = vline_data[vline].front()->node()->isSelected();

    auto pixel_count = vline_data[vline].size();

    std::vector<int> intencity_vec(max_depth + 1, 0);

    /// fill the intencity vector
    for (const auto& pixel_item : vline_data[vline]) {
      intencity_vec.at(pixel_item->depth())++;
    }

    for (auto depth = 0; depth < intencity_vec.size(); depth++) {
      if (intencity_vec[depth] == 0) continue; /// no pixel at that depth
      auto y = depth - yoff;
      if (y > img_height || y < 0) continue; /// out of image boundary
      // pixel_image.drawPixel(x, y, PixelImage::PIXEL_COLOR::BLACK_ALPHA);
      int value = 100 - 100 * static_cast<float>(intencity_vec[depth]) / pixel_count;
      if (!isSelected)
        pixel_image.drawPixel(x, y, QColor::fromHsv(0, 0, value).rgba());
      else
        pixel_image.drawPixel(x, y, QColor::fromHsv(300, 255, 255).rgba());
    }
  }

}

/// Make sure no redundant work is done
void
PixelTreeCanvas::redrawAll() {

  pixel_image.clear();

  auto scale = pixel_image.scale();
  auto img_width = pixel_image.image().width();

  auto vlines = pixel_data.compressed_list.size();

  // _sa->horizontalScrollBar()->setRange(0, vlines - img_width / scale + 20);
  _sa->verticalScrollBar()->setRange(0, max_depth +
    5 * (HIST_HEIGHT + MARGIN) - _sa->height()); // 4 histograms

  int xoff = _sa->horizontalScrollBar()->value();
  int yoff = _sa->verticalScrollBar()->value();

  unsigned int leftmost_x = xoff;
  unsigned int rightmost_x = xoff + _sa->width();

  if (rightmost_x > pixel_data.pixel_list.size() * scale) {
    rightmost_x = pixel_data.pixel_list.size() * scale;
  }

  this->resize(pixel_image.image().width(), pixel_image.image().height());

  pixel_image.drawGrid(xoff, yoff);

  drawPixelTree(pixel_data);

  /// All Histograms

  drawTimeHistogram(2);

  drawDomainHistogram(3);

  // drawDomainReduction(image, leftmost_vline, rightmost_vline);

  // drawNodeRate(image, leftmost_vline, rightmost_vline);

  drawDepthAnalysisData();

  drawVarData();

  repaint();

}

void
PixelTreeCanvas::drawVarData() {

  auto xoff = _sa->horizontalScrollBar()->value();
  auto yoff = _sa->verticalScrollBar()->value();

  auto img_width = pixel_image.image().width();
  auto img_height = pixel_image.image().height();

  auto scale = pixel_image.scale();

  int zero_level = max_depth + 4 * ceil((HIST_HEIGHT + MARGIN) / scale);
  pixel_image.drawHorizontalLine(zero_level);

  for (auto vline = 0; vline < var_decisions_compressed.size(); vline++) {
    for (auto i = 0; i < var_decisions_compressed.at(vline).size(); i++) {

      auto var_id = var_decisions_compressed[vline][i];

      auto x = vline - xoff;
      auto y = zero_level - var_id - yoff;

      if (x > img_width) break;
      if (y > img_width || y < 0) continue;

      auto color_value = ceil(var_id * 255 / vars.size());

      pixel_image.drawPixel(x, y, QColor::fromHsv(color_value, 200, 255).rgba());

    }
  }
}

/// Draw time histogram underneath the pixel tree
void
PixelTreeCanvas::drawTimeHistogram(int id) {

  drawHistogram(id, time_arr, qRgb(150, 150, 40));
}

void
PixelTreeCanvas::drawDomainHistogram(int id) {
  drawHistogram(id, domain_arr, qRgb(150, 40, 150));
}

void
PixelTreeCanvas::drawDomainReduction(unsigned l_vline, unsigned r_vline) {
  // drawHistogram(2, domain_red_arr, l_vline, r_vline, qRgb(40, 150, 150));
}

void
PixelTreeCanvas::drawHistogram(int idx, vector<float>& data, int color) {

  auto xoff = _sa->horizontalScrollBar()->value();
  auto yoff = _sa->verticalScrollBar()->value();

  auto img_width = pixel_image.image().width();
  auto img_height = pixel_image.image().height();

  /// work out maximum value
  int max_value = *max_element(std::begin(data), std::end(data));

  if (max_value <= 0) { /// no data for this histogram
    qDebug() << "(!) no data for the time histogram";
    return;
  }

  auto scale = pixel_image.scale();

  float coeff = static_cast<float>(HIST_HEIGHT / scale) / max_value;

  int zero_level = max_depth + idx * ceil((HIST_HEIGHT + MARGIN) / scale);

  pixel_image.drawHorizontalLine(zero_level);
  pixel_image.drawHorizontalLine(zero_level - ceil(HIST_HEIGHT / scale) - 1);

  for (auto vline = 0; vline < data.size(); vline++) {
    auto val = floor(data[vline] * coeff);

    auto x = vline - xoff;
    auto y = zero_level - val - yoff;

    if (x > img_width) break;
    if (y > img_width || y < 0) continue;

    pixel_image.drawPixel(x, y, color);


  }

}

void
PixelTreeCanvas::drawNodeRate(unsigned l_vline, unsigned r_vline) {

  std::vector<float>& node_rate = _data.node_rate;
  std::vector<int>& nr_intervals = _data.nr_intervals;

  int start_x = 0;

  float max_node_rate = *std::max_element(node_rate.begin(), node_rate.end());

  float coeff = (float)HIST_HEIGHT / max_node_rate;

  int zero_level = max_depth + HIST_HEIGHT;

  pixel_image.drawHorizontalLine(zero_level);

  for (unsigned i = 1; i < nr_intervals.size(); i++) {
    float value = node_rate[i - 1] * coeff;
    unsigned i_begin = ceil((float)nr_intervals[i-1] / approx_size);
    unsigned i_end   = ceil((float)nr_intervals[i]   / approx_size);

    /// draw this interval?
    if (i_end < l_vline || i_begin > r_vline)
      continue;

    if (i_begin < l_vline) i_begin = l_vline;
    if (i_end   > r_vline) i_end   = r_vline;

    /// this is very slow
    // for (unsigned x = i_begin; x < i_end; x++) {

    //   for (int v = value; v >= 0; v--) {
    //     drawPixel(start_x + (x - l_vline) * _step,
    //               start_y + HIST_HEIGHT - v,
    //               qRgb(40, 40, 150));
    //   }
    // }
  }
}

void
PixelTreeCanvas::drawDepthAnalysisData() {

  auto scale = pixel_image.scale();

  int zero_level = max_depth + 1 * ceil((HIST_HEIGHT + MARGIN) / scale);

  pixel_image.drawHorizontalLine(zero_level);

  auto xoff = _sa->horizontalScrollBar()->value(); // values should be in scaled pixels
  auto yoff = _sa->verticalScrollBar()->value();

  auto img_width = pixel_image.image().width();
  auto img_height = pixel_image.image().height();

  /// TODO: allow any height

  /// max value:
  // auto max_value = 0;
  // for (auto depth = 0; depth < da_data_compressed.size(); depth++) {
  //   for (auto vline = xoff; vline < da_data_compressed[depth].size(); vline++) {
  //     auto value = da_data_compressed[depth][vline];
  //     if (value > max_value) max_value = value;
  //   }
  // }

  /// *** Actual Data ***
  /// for each depth level:
  for (auto depth = 0; depth < da_data_compressed.size(); depth++) {

    for (auto vline = xoff; vline < da_data_compressed[depth].size(); vline++) {

      auto value = da_data_compressed[depth][vline];
      auto x = vline - xoff;
      auto y = zero_level - value - yoff;

      int color_value = 200 - 200 * static_cast<float>(depth) / max_depth;

      if (value != 0)
      pixel_image.drawPixel(x, y,  QColor::fromHsv(180, 180, color_value).rgba());
    }

  }

}

void
PixelTreeCanvas::scaleUp(void) {

  pixel_image.scaleUp();
  redrawAll();
  
}

void
PixelTreeCanvas::scaleDown(void) {

  pixel_image.scaleDown();
  redrawAll();
}

void
PixelTreeCanvas::resizeCanvas(void) {
  // pixel_image

  auto width = _sa->viewport()->width();
  auto height =  _sa->viewport()->height();
  pixel_image.resize(width, height);
  redrawAll();
}

void
PixelTreeCanvas::compressionChanged(int value) {
  compressPixelTree(value);
  compressDepthAnalysis(da_data_compressed, value);
  compressTimeHistogram(time_arr, value);
  compressDomainHistogram(domain_arr, value);
  compressVarData(var_decisions_compressed, value);
  redrawAll();
}

void
PixelTreeCanvas::sliderChanged(int value) {
  redrawAll();
}

void
PixelTreeCanvas::mousePressEvent(QMouseEvent* event) {

  auto xoff = _sa->horizontalScrollBar()->value();
  auto yoff = _sa->verticalScrollBar()->value();

  auto vline = event->x() / pixel_image.scale() + xoff;
  auto y = event->y() + yoff;

  /// check boundaries


  selectNodesfromPT(vline);
  redrawAll();

}

void
PixelTreeCanvas::selectNodesfromPT(unsigned vline) {

  struct Actions {

  private:
    NodeAllocator* _na;
    TreeCanvas& _tc;
    
    bool _done;

  public:

    void selectOne(VisualNode* node) {
      _tc.setCurrentNode(node);
      _tc.centerCurrentNode();
    }

    void selectGroup(VisualNode* node) {
      node->dirtyUp(*_na);

      VisualNode* next = node;

      while (!next->isRoot() && next->isHidden()) {
        next->setHidden(false);
        next = next->getParent(*_na);
      }
    }

  public:

    Actions(NodeAllocator* na, TreeCanvas& tc)
    : _na(na), _tc(tc), _done(false) {}

  };


  /// TODO: Add clickability

  // /// select the last one in case clicked a bit off the boundary
  // vline = (pixelList.size() > vline) ? vline : pixelList.size() - 1;

  if (vline >= pixel_data.compressed_list.size()) return;

  // qDebug() << "selecting vline: " << vline;

  Actions actions(_na, _tc);
  void (Actions::*apply)(VisualNode*);

  // /// unset currently selected nodes
  for (auto& node : nodes_selected) {
    node->setSelected(false);
  }

  nodes_selected.clear();

  auto vline_list = pixel_data.compressed_list[vline];

  if (vline_list.size() == 1) {
    apply = &Actions::selectOne;
  } else {
    apply = &Actions::selectGroup;

    /// hide everything except root
    _tc.hideAll();
    (*_na)[0]->setHidden(false);
  }

  for (auto& pixel : vline_list) {
    (actions.*apply)(pixel->node());
    pixel->node()->setSelected(true);
    nodes_selected.push_back(pixel->node());
  }

  
  _tc.update();

}

/// ***********************************
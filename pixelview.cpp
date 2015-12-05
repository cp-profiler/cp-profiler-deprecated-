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

  time_arr.clear();       time_arr.resize(vlines);
  domain_arr.clear();     domain_arr.resize(vlines);
  domain_red_arr.clear(); domain_red_arr.resize(vlines);


  /// get a root
  VisualNode* root = (*_na)[0];

  vline_idx    = 0;
  node_idx     = 0;
  group_size   = 0;
  group_domain = 0;
  group_domain_red    = 0;
  group_size_nonempty = 0;

  alpha_factor = 100.0 / approx_size;

  pixel_data = traverseTree(root);
  // traverseTreePostOrder(root);

  flush();

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

    // processCurrentNode(node, depth);
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

void
PixelTreeCanvas::traverseTreePostOrder(VisualNode* root) {

  std::stack<VisualNode*> nodeStack1;
  std::stack<unsigned int> depthStack1;

  std::stack<VisualNode*> nodeStack2;
  std::stack<unsigned int> depthStack2;

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

    processCurrentNode(node, depth);
  }
}



void PixelTreeCanvas::processCurrentNode(VisualNode* node, unsigned int depth) {
  DbEntry* entry = _data.getEntry(node->getIndex(*_na));
}

/// Needs:
/// pixelList, vline_idx, max_depth, node_idx, group_size_nonempty
/// group_domain_red, group_time, group_domain, group_size, approx_size,
/// time_arr, domain_arr, domain_red_arr
/// on class scope...
// void PixelTreeCanvas::processCurrentNode_old(VisualNode* node, unsigned int depth) {

//   /// 2.3 apply the action to the next node in a while loop

//   DbEntry* entry = _data.getEntry(node->getIndex(*_na));

//   assert(depth <= max_depth);

//   pixelList[vline_idx].push_back(PixelItem(node_idx, node, depth));

//   if (vline_idx >= pixelList.size()) return;

//   if (entry) {

//     group_size_nonempty++;

//     if (entry->parent_sid != ~0u) {
//       DbEntry* parent = _data.getEntry(node->getParent());

//       if (parent) /// need this for restarts
//         group_domain_red += parent->domain - entry->domain;
//     }

//     group_time   += entry->node_time;
//     group_domain += entry->domain;


//   }

//   group_size++;

//   if (group_size == approx_size) {
//     vline_idx++;
//     group_size = 0;


//     /// get average domain size for the group
//     if (group_size_nonempty == 0) {
//       group_domain      = -1;
//       group_domain_red  = -1;
//       group_time        = -1;
//     } else {
//       group_domain        = group_domain / group_size_nonempty;
//       group_domain_red    = group_domain_red / group_size_nonempty;
      
//     }


//     time_arr[vline_idx]       = group_time;
//     domain_arr[vline_idx]     = group_domain;
//     domain_red_arr[vline_idx] = group_domain_red;
    
//     group_time   = 0;
//     group_domain = 0;
//     group_domain_red = 0;
//     group_size_nonempty = 0;

//   }

//   node_idx++;
// }


/// TODO:
/// 1. be able to compress
/// 2. show selected differently
/// 3. show solutions
void
PixelTreeCanvas::drawPixelTree(const PixelData& pixel_data) {

  auto xoff = _sa->horizontalScrollBar()->value(); // values should be in scaled pixels
  auto yoff = _sa->verticalScrollBar()->value();

  auto img_width = pixel_image.image().width();
  auto img_height = pixel_image.image().height();

  const auto& vline_data = pixel_data.compressed_list;

  /// TODO: 2. Draw vertical lines from solutions

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
      pixel_image.drawPixel(x, y, QColor::fromHsv(0, 0, value).rgba());
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

  _sa->horizontalScrollBar()->setRange(0, vlines - img_width / scale + 20);
  _sa->verticalScrollBar()->setRange(0, max_depth +
    5 * (HIST_HEIGHT + MARGIN) - _sa->height()); // 4 histograms

  int xoff = _sa->horizontalScrollBar()->value();
  int yoff = _sa->verticalScrollBar()->value();

  unsigned int leftmost_x = xoff;
  unsigned int rightmost_x = xoff + _sa->width();

  pt_height = max_depth;

  if (rightmost_x > pixel_data.pixel_list.size() * scale) {
    rightmost_x = pixel_data.pixel_list.size() * scale;
  }

  this->resize(pixel_image.image().width(), pixel_image.image().height());

  pixel_image.drawGrid(xoff, yoff);

  // unsigned leftmost_vline = leftmost_x / _step;
  // unsigned rightmost_vline = rightmost_x / _step;

  int node_idx = 0;

  /// TODO: do not use this 'global':

  drawPixelTree(pixel_data);

  // for (unsigned int vline = leftmost_vline; vline < rightmost_vline; vline++) {
  //   if (!pixelList[vline].empty()) {

  //     std::fill(intencity_arr.begin(), intencity_arr.end(), 0);

  //     for (auto& pixel : pixelList[vline]) {


  //       int xpos = (vline  - leftmost_vline) * _step;
  //       int ypos = pixel.depth() * _step_y - yoff;


  //       intencity_arr.at(pixel.depth())++;

  //       /// draw pixel itself:
  //       if (ypos > 0) {
  //         if (!pixel.node()->isSelected()) {
  //           int alpha = intencity_arr.at(pixel.depth()) * alpha_factor;
  //           drawPixel(xpos, ypos, QColor::fromHsv(150, 100, 100 - alpha).rgba());
  //         } else {
  //           // drawPixel(xpos, ypos, qRgb(255, 0, 0));
  //           drawPixel(xpos, ypos, qRgb(255, 0, 255));
  //         }
  //       }
        
  //       /// draw green vertical line if solved:
  //       if (pixel.node()->getStatus() == SOLVED) {

  //         for (unsigned j = 0; j < pt_height - yoff; j++)
  //           if (image.pixel(xpos, j) == qRgb(255, 255, 255))
  //             for (unsigned i = 0; i < _step; i++)
  //               image.setPixel(xpos + i, j, qRgb(0, 255, 0));

  //       }

  //       node_idx++;
  //     }
      
  //   }
  // }

  /// All Histograms

  // drawTimeHistogram(image, leftmost_vline, rightmost_vline);

  // drawDomainHistogram(image, leftmost_vline, rightmost_vline);

  // drawDomainReduction(image, leftmost_vline, rightmost_vline);

  // drawNodeRate(image, leftmost_vline, rightmost_vline);

  drawDepthAnalysisData();

  repaint();

}

void
PixelTreeCanvas::flush(void) {

  if (group_size == 0)
    return;

  if (group_size_nonempty == 0) {
    group_domain      = -1;
    group_domain_red  = -1;
    group_time        = -1;
  } else {
    group_domain        = group_domain / group_size_nonempty;
    group_domain_red    = group_domain_red / group_size_nonempty;
  }

  domain_arr[vline_idx]     = group_domain;
  domain_red_arr[vline_idx] = group_domain_red;
  time_arr[vline_idx]       =  group_time;

}

/// Draw time histogram underneath the pixel tree
void
PixelTreeCanvas::drawTimeHistogram(unsigned l_vline, unsigned r_vline) {

  drawHistogram(0, time_arr, l_vline, r_vline, qRgb(150, 150, 40));
}

void
PixelTreeCanvas::drawDomainHistogram(unsigned l_vline, unsigned r_vline) {
  drawHistogram(1, domain_arr, l_vline, r_vline, qRgb(150, 40, 150));
}

void
PixelTreeCanvas::drawDomainReduction(unsigned l_vline, unsigned r_vline) {
  drawHistogram(2, domain_red_arr, l_vline, r_vline, qRgb(40, 150, 150));
}

void
PixelTreeCanvas::drawHistogram(int idx, vector<float>& data, unsigned l_vline, unsigned r_vline, int color) {

  /// coordinates for the top-left corner
  int init_x = 0;
  int yoff = _sa->verticalScrollBar()->value();
  int y = (pt_height) + MARGIN + idx * (HIST_HEIGHT + MARGIN) - yoff;

  /// work out maximum value
  int max_value = 0;

  for (unsigned i = 0; i < vlines; i++) {
    if (data[i] > max_value) max_value = data[i];
  }

  if (max_value <= 0) return; /// no data for this histogram

  float coeff = (float)HIST_HEIGHT / max_value;

  int zero_level = y + HIST_HEIGHT;

  for (unsigned i = l_vline; i < r_vline; i++) {
    int val = data[i] * coeff;

    /// horizontal line for 0 level
    // for (unsigned j = 0; j < _step; j++)
    //   image.setPixel(init_x + (i - l_vline) * _step + j,
    //                    zero_level, 
    //                    qRgb(150, 150, 150));

    // if (data[i] < 0) continue;

    for (int v = val; v >= 0; v--) {
      pixel_image.drawPixel(init_x + (i - l_vline),
                y + HIST_HEIGHT - v,
                color);
    }

  }


}

void
PixelTreeCanvas::drawNodeRate(unsigned l_vline, unsigned r_vline) {

  std::vector<float>& node_rate = _data.node_rate;
  std::vector<int>& nr_intervals = _data.nr_intervals;

  int start_x = 0;
  int start_y = (pt_height) + MARGIN + 3 * (HIST_HEIGHT + MARGIN );

  float max_node_rate = *std::max_element(node_rate.begin(), node_rate.end());

  float coeff = (float)HIST_HEIGHT / max_node_rate;

  int zero_level = start_y + HIST_HEIGHT;

  // // / this is very slow
  // for (unsigned i = l_vline; i < r_vline; i++) {
  //   for (unsigned j = 0; j < _step; j++)
  //     image.setPixel(start_x + (i - l_vline) * _step + j,
  //                      zero_level, 
  //                      qRgb(150, 150, 150));
  // }

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

  int start_x = 0;
  int start_y = (pt_height) + MARGIN;

  // /// add step twice because we want the line to be at the bottom
  int zero_level = start_y + HIST_HEIGHT;


  /// zero level line

  // for (unsigned i = l_vline; i < r_vline; i++) {
  //   for (unsigned j = 0; j < _step; j++) {

  //     image.setPixel(start_x + (i - l_vline) * _step + j,
  //                      zero_level, 
  //                      qRgb(150, 150, 150));


  //   }
  // }

  auto xoff = _sa->horizontalScrollBar()->value(); // values should be in scaled pixels
  auto yoff = _sa->verticalScrollBar()->value();

  auto img_width = pixel_image.image().width();
  auto img_height = pixel_image.image().height();

  /// *** Actual Data ***
  /// for each depth level:
  for (auto depth = 0; depth < da_data.size(); depth++) {

    for (auto vline = xoff; vline < da_data[depth].size(); vline++) {

      int value = da_data[depth][vline];
      auto x = vline - xoff;
      auto y = zero_level - value - yoff;

      // if (value != 0)
      pixel_image.drawPixel(x, y, qRgb(depth * 5, 0, 255));
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

  auto x = event->x() + xoff;
  auto y = event->y() + yoff;

  /// check boundaries:
  if (y > pt_height) return;

  // which node?

  unsigned vline = x / pixel_image.scale();

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

  // /// select the last one in case clicked a bit off the boundary
  // vline = (pixelList.size() > vline) ? vline : pixelList.size() - 1;

  // qDebug() << "selecting vline: " << vline;

  // Actions actions(_na, _tc);
  // void (Actions::*apply)(VisualNode*);

  // /// unset currently selected nodes
  // for (auto& node : nodes_selected) {
  //   node->setSelected(false);
  // }

  // nodes_selected.clear();

  // std::list<PixelItem>& vline_list = pixelList[vline];

  // if (vline_list.size() == 1) {
  //   apply = &Actions::selectOne;
  // } else {
  //   apply = &Actions::selectGroup;

  //   /// hide everything except root
  //   _tc.hideAll();
  //   (*_na)[0]->setHidden(false);
  // }

  // for (auto& pixel : vline_list) {
  //   (actions.*apply)(pixel.node());
  //   pixel.node()->setSelected(true);
  //   nodes_selected.push_back(pixel.node());
  // }

  
  // _tc.update();

}

/// ***********************************
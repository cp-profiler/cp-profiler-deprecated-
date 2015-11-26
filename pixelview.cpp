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
using std::vector;

/// ******* PIXEL_TREE_DIALOG ********

PixelTreeDialog::PixelTreeDialog(TreeCanvas* tc)
  : QDialog(tc)
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

  canvas = new PixelTreeCanvas(&scrollArea, tc);

  connect(scaleDown, SIGNAL(clicked()), canvas, SLOT(scaleDown()));
  connect(scaleUp, SIGNAL(clicked()), canvas, SLOT(scaleUp()));
  connect(compressionSB, SIGNAL(valueChanged(int)),
    canvas, SLOT(compressionChanged(int)));



  setAttribute(Qt::WA_QuitOnClose, true);
  setAttribute(Qt::WA_DeleteOnClose, true);

}

PixelTreeDialog::~PixelTreeDialog(void) {
  delete canvas;
}

PixelTreeCanvas::~PixelTreeCanvas(void) {
  delete _image;
}

/// ***********************************


/// ******** PIXEL_TREE_CANVAS ********

PixelTreeCanvas::PixelTreeCanvas(QWidget* parent, TreeCanvas* tc)
  : QWidget(parent), _tc(tc), _na(tc->na), depthAnalysis(tc)
{

  _sa = static_cast<QAbstractScrollArea*>(parentWidget());
  _vScrollBar = _sa->verticalScrollBar();

  _nodeCount = tc->stats.solutions + tc->stats.failures
                       + tc->stats.choices + tc->stats.undetermined;

  max_depth = tc->stats.maxDepth;

  // if (_tc->getData()->isRestarts()) {
  //   max_depth++; /// consider the first, dummy node
  //   _nodeCount++;
  // }

  _image = nullptr;

  /// scrolling business
  _sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  _sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  // _sa->setAutoFillBackground(true);

  da_data = depthAnalysis.runMSL();
  
  constructTree();


}

void
PixelTreeCanvas::paintEvent(QPaintEvent*) {
  QPainter painter(this);

  drawPixelTree();
  /// start at (1;1) to prevent strage artifact
  painter.drawImage(1, 1, *_image, 0, 0, _sa->viewport()->width(), _sa->viewport()->height());
}


void
PixelTreeCanvas::constructTree(void) {

  high_resolution_clock::time_point time_begin = high_resolution_clock::now();

  /// how many of each values after compression
  vlines = ceil((float)_nodeCount / approx_size);

  pixelList.clear();      pixelList.resize(vlines);

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

  // traverseTree(root);
  traverseTreePostOrder(root);

  flush();

  high_resolution_clock::time_point time_end = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(time_end - time_begin);
  std::cout << "Pixel Tree construction took: " << time_span.count() << " seconds." << std::endl;

}

void
PixelTreeCanvas::drawPixelTree() {
  delete _image;

  _sa->horizontalScrollBar()->setRange(0, vlines * _step - _sa->width() + 100);
  _sa->verticalScrollBar()->setRange(0, max_depth * _step +
    5 * (HIST_HEIGHT + MARGIN + _step) - _sa->height()); // 4 histograms

  int xoff = _sa->horizontalScrollBar()->value();
  int yoff = _sa->verticalScrollBar()->value();

  unsigned int leftmost_x = xoff;
  unsigned int rightmost_x = xoff + _sa->width();

  pt_height = max_depth * _step_y;

  qDebug() << "pixelList.size(): " << pixelList.size();

  if (rightmost_x > pixelList.size() * _step) {
    rightmost_x = pixelList.size() * _step;
  }

  int img_height = MARGIN + 
                   pt_height + _step +
                   MARGIN +
                   HIST_HEIGHT + _step + /// Time Histogram
                   MARGIN +
                   HIST_HEIGHT + _step + /// Domain Histogram
                   MARGIN +
                   HIST_HEIGHT + _step + /// Domain Reduction Histogram
                   MARGIN +
                   HIST_HEIGHT + _step + /// Node Rate Histogram
                   MARGIN +
                   HIST_HEIGHT + _step + /// Depth Analysis
                   MARGIN;

  // _image = new QImage(rightmost_x - leftmost_x + _step, img_height, QImage::Format_RGB888);
  _image = new QImage(rightmost_x - leftmost_x + _step, img_height, QImage::Format_RGB888);

  _image->fill(qRgb(255, 255, 255));

  this->resize(_image->width(), _image->height());

  drawGrid(xoff, yoff);

  unsigned leftmost_vline = leftmost_x / _step;
  unsigned rightmost_vline = rightmost_x / _step;

  std::vector<int> intencity_arr(max_depth + 1, 0);

  int node_idx = 0;

  for (unsigned int vline = leftmost_vline; vline < rightmost_vline; vline++) {
    if (!pixelList[vline].empty()) {

      std::fill(intencity_arr.begin(), intencity_arr.end(), 0);

      for (auto& pixel : pixelList[vline]) {


        int xpos = (vline  - leftmost_vline) * _step;
        int ypos = pixel.depth() * _step_y - yoff;


        intencity_arr.at(pixel.depth())++;

        /// draw pixel itself:
        if (ypos > 0) {
          if (!pixel.node()->isSelected()) {
            int alpha = intencity_arr.at(pixel.depth()) * alpha_factor;
            drawPixel(xpos, ypos, QColor::fromHsv(150, 100, 100 - alpha).rgba());
          } else {
            // drawPixel(xpos, ypos, qRgb(255, 0, 0));
            drawPixel(xpos, ypos, qRgb(255, 0, 255));
          }
        }
        
        /// draw green vertical line if solved:
        if (pixel.node()->getStatus() == SOLVED) {

          for (unsigned j = 0; j < pt_height - yoff; j++)
            if (_image->pixel(xpos, j) == qRgb(255, 255, 255))
              for (unsigned i = 0; i < _step; i++)
                _image->setPixel(xpos + i, j, qRgb(0, 255, 0));

        }

        node_idx++;
      }
      
    }
  }

  /// All Histograms

  // drawTimeHistogram(leftmost_vline, rightmost_vline);

  // drawDomainHistogram(leftmost_vline, rightmost_vline);

  // drawDomainReduction(leftmost_vline, rightmost_vline);

  // drawNodeRate(leftmost_vline, rightmost_vline);

  drawDepthAnalysisData(leftmost_vline, rightmost_vline);
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


void
PixelTreeCanvas::traverseTree(VisualNode* root) {

  /// 0. prepare a stack for exploration
  std::stack<VisualNode*> explorationStack;
  std::stack<unsigned int> depthStack;

  /// 1. push the root node
  explorationStack.push(root);
  depthStack.push(1);

  /// 2. traverse the stack
  while(explorationStack.size() > 0) {

    VisualNode* node   = explorationStack.top(); explorationStack.pop();
    unsigned int depth = depthStack.top();       depthStack.pop();

    processCurrentNode(node, depth);

    /// 2.1. add the children to the stack

    uint kids = node->getNumberOfChildren();
    for (uint i = 0; i < kids; ++i) {
      auto kid = node->getChild(*_na, i);
      explorationStack.push(kid);
      depthStack.push(depth + 1);
    }

  }
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

  /// 2.3 apply the action to the next node in a while loop

  Data* data = _tc->getData();
  DbEntry* entry = data->getEntry(node->getIndex(*_na));
  DbEntry* parent = nullptr;

  assert(depth <= max_depth);

  pixelList[vline_idx].push_back(PixelData(node_idx, node, depth));

  if (vline_idx >= pixelList.size()) return;

  if (entry) {

    group_size_nonempty++;

    if (entry->parent_sid != ~0u) {
      parent = data->getEntry(node->getParent());

      if (parent) /// need this for restarts
        group_domain_red += parent->domain - entry->domain;
    }

    group_time   += entry->node_time;
    group_domain += entry->domain;


  }

  group_size++;

  if (group_size == approx_size) {
    vline_idx++;
    group_size = 0;


    /// get average domain size for the group
    if (group_size_nonempty == 0) {
      group_domain      = -1;
      group_domain_red  = -1;
      group_time        = -1;
    } else {
      group_domain        = group_domain / group_size_nonempty;
      group_domain_red    = group_domain_red / group_size_nonempty;
      
    }


    time_arr[vline_idx]       = group_time;
    domain_arr[vline_idx]     = group_domain;
    domain_red_arr[vline_idx] = group_domain_red;
    
    group_time   = 0;
    group_domain = 0;
    group_domain_red = 0;
    group_size_nonempty = 0;

  }

  node_idx++;
}

void PixelTreeCanvas::drawGrid(unsigned int xoff, unsigned int yoff) {

  /// draw cells 5 squares wide
  int gap =  2;
  int gap_size = gap * _step; /// actual gap size in pixels

  /// horizontal lines on level == j
  for (unsigned int j = gap_size; j < _image->height(); j += gap_size) {

    /// one line
    for (unsigned int i = 0; i < _image->width() - _step; i++) {

      for (unsigned k = 0; k < _step; k++)
        _image->setPixel(i + k, j, qRgb(200, 200, 200));

    }
  }

  /// vertical lines on column == i
  for (unsigned int i = gap_size; i < _image->width(); i += gap_size) {

    /// one line
    for (unsigned int j = 0; j < _image->height() - _step; j++) {

      for (unsigned k = 0; k < _step; k++)
        _image->setPixel(i, j + k, qRgb(200, 200, 200));

    }
  }

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
  int y = (pt_height + _step) + MARGIN + idx * (HIST_HEIGHT + MARGIN + _step) - yoff;

  /// work out maximum value
  int max_value = 0;

  for (unsigned i = 0; i < vlines; i++) {
    if (data[i] > max_value) max_value = data[i];
  }

  if (max_value <= 0) return; /// no data for this histogram

  float coeff = (float)HIST_HEIGHT / max_value;

  int zero_level = y + HIST_HEIGHT + _step;

  for (unsigned i = l_vline; i < r_vline; i++) {
    int val = data[i] * coeff;

    /// horizontal line for 0 level
    for (unsigned j = 0; j < _step; j++)
      _image->setPixel(init_x + (i - l_vline) * _step + j,
                       zero_level, 
                       qRgb(150, 150, 150));

    // if (data[i] < 0) continue;

    for (int v = val; v >= 0; v--) {
      drawPixel(init_x + (i - l_vline) * _step,
                y + HIST_HEIGHT - v,
                color);
    }

  }


}

void
PixelTreeCanvas::drawNodeRate(unsigned l_vline, unsigned r_vline) {
  Data* data = _tc->getData();
  std::vector<float>& node_rate = data->node_rate;
  std::vector<int>& nr_intervals = data->nr_intervals;

  int start_x = 0;
  int start_y = (pt_height + _step) + MARGIN + 3 * (HIST_HEIGHT + MARGIN + _step);

  float max_node_rate = *std::max_element(node_rate.begin(), node_rate.end());

  float coeff = (float)HIST_HEIGHT / max_node_rate;

  int zero_level = start_y + HIST_HEIGHT + _step;

  // / this is very slow
  for (unsigned i = l_vline; i < r_vline; i++) {
    for (unsigned j = 0; j < _step; j++)
      _image->setPixel(start_x + (i - l_vline) * _step + j,
                       zero_level, 
                       qRgb(150, 150, 150));
  }

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
PixelTreeCanvas::drawDepthAnalysisData(unsigned l_vline, unsigned r_vline) {

  int start_x = 0;
  int start_y = (pt_height + _step) + MARGIN + 4 * (HIST_HEIGHT + MARGIN + _step);

  /// add step twice because we want the line to be at the bottom
  int zero_level = start_y + HIST_HEIGHT + _step + _step;


  /// zero level line

  for (unsigned i = l_vline; i < r_vline; i++) {
    for (unsigned j = 0; j < _step; j++) {

      _image->setPixel(start_x + (i - l_vline) * _step + j,
                       zero_level, 
                       qRgb(150, 150, 150));


    }
  }

  qDebug() << "da_data[0].size(): " << da_data[0].size();

  /// actual data

  for (unsigned int depth = 0; depth < da_data.size(); depth++) {

    /// set the limit
    unsigned int r_limit = std::min(r_vline, static_cast<unsigned int>(da_data[depth].size()));

    for (unsigned i = l_vline; i < r_limit; i++) {

      int value = da_data[depth][i];
      int xpos = start_x + (i - l_vline) * _step;
      int ypos = zero_level - value * _step;

      // qDebug() << "da_data[" << depth << "][" << i << "]: " << value;

      // if (value != 0)
        drawPixel(xpos, ypos, qRgb(depth * 5, 0, 255));
    }

  }

}

void
PixelTreeCanvas::scaleUp(void) {
  _step++;
  _step_y++;
  drawPixelTree();
  repaint();
}

void
PixelTreeCanvas::scaleDown(void) {
  if (_step <= 1) return;
  _step--;
  _step_y--;
  drawPixelTree();
  repaint();
}

void
PixelTreeCanvas::compressionChanged(int value) {
  approx_size = value;
  constructTree();
  repaint();
}

void
PixelTreeCanvas::drawPixel(int x, int y, int color) {
  if (y < 0)
    return; /// TODO: fix later

  for (unsigned i = 0; i < _step; i++)
    for (unsigned j = 0; j < _step_y; j++)
      _image->setPixel(x + i, y + j, color);

}

void
PixelTreeCanvas::mousePressEvent(QMouseEvent* me) {

  int xoff = _sa->horizontalScrollBar()->value();
  int yoff = _sa->verticalScrollBar()->value();

  unsigned x = me->x() + xoff;
  unsigned y = me->y() + yoff;

  /// check boundaries:
  if (y > pt_height) return;

  // which node?

  unsigned vline = x / _step;

  selectNodesfromPT(vline);

  drawPixelTree();
  repaint();

}

void
PixelTreeCanvas::selectNodesfromPT(unsigned vline) {

  struct Actions {

  private:
    NodeAllocator* _na;
    TreeCanvas* _tc;
    int node_id;
    
    bool _done;

  public:

    void selectOne(VisualNode* node) {
      _tc->setCurrentNode(node);
      _tc->centerCurrentNode();
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

    Actions(NodeAllocator* na, TreeCanvas* tc)
    : _na(na), _tc(tc), _done(false) {}

  };

  /// select the last one in case clicked a bit off the boundary
  vline = (pixelList.size() > vline) ? vline : pixelList.size() - 1;

  qDebug() << "selecting vline: " << vline;

  Actions actions(_na, _tc);
  void (Actions::*apply)(VisualNode*);

  /// unset currently selected nodes
  for (auto& node : nodes_selected) {
    node->setSelected(false);
  }

  nodes_selected.clear();

  std::list<PixelData>& vline_list = pixelList[vline];

  if (vline_list.size() == 1) {
    apply = &Actions::selectOne;
  } else {
    apply = &Actions::selectGroup;

    /// hide everything except root
    _tc->hideAll();
    (*_na)[0]->setHidden(false);
  }

  for (auto& pixel : vline_list) {
    (actions.*apply)(pixel.node());
    pixel.node()->setSelected(true);
    nodes_selected.push_back(pixel.node());
  }

  
  _tc->update();

}

/// ***********************************
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

#include <stack>

#include "pixel_tree_canvas.hh"
#include "cpprofiler/analysis/backjumps.hh"

using namespace cpprofiler::pixeltree;
// using namespace std::chrono;
// using std::vector; using std::list;


static std::pair<int, int>
getPixelBoundaries(int vline, int compression) {
  return std::make_pair(vline * compression, (vline + 1) * compression);
}


/// ******** PIXEL_TREE_CANVAS ********

PixelTreeCanvas::PixelTreeCanvas(QWidget* parent, TreeCanvas& tc)
    : QWidget(parent), _tc(tc), _data(*tc.getExecution()->getData()), _na(tc.get_na()), depthAnalysis(tc)
{



  _sa = static_cast<QAbstractScrollArea*>(parentWidget());
  _vScrollBar = _sa->verticalScrollBar();

  _nodeCount = tc.get_stats().solutions + tc.get_stats().failures
                       + tc.get_stats().choices + tc.get_stats().undetermined;

  tree_depth = tc.get_stats().maxDepth;

  // if (_tc->getData()->isRestarts()) {
  //   tree_depth++; /// consider the first, dummy node
  //   _nodeCount++;
  // }

  /// scrolling business
  _sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  _sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  // _sa->setAutoFillBackground(true);

  setMouseTracking(true);

  connect (_sa->horizontalScrollBar(), SIGNAL(valueChanged (int)), this, SLOT(sliderChanged(int)));
  connect (_sa->verticalScrollBar(), SIGNAL(valueChanged (int)), this, SLOT(sliderChanged(int)));

  da_data = depthAnalysis.runMSL();

  typename cpprofiler::analysis::Backjumps bj;
  bj_data = bj.findBackjumps((*_na)[0], *_na);
  
  constructPixelTree();
  compressPixelTree(1);
  compressDepthAnalysis(da_data_compressed, 1);
  compressTimeHistogram(time_arr, 1);
  compressDomainHistogram(domain_arr, 1);
  gatherVarData();
  compressVarData(var_decisions_compressed, 1);
  gatherNogoodData();
  compressNogoodData(1);
  resizeCanvas();

  // redrawAll();

}

void
PixelTreeCanvas::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  /// start at (1;1) to prevent strage artifact
  if (pixel_image.image() == nullptr) return;
  painter.drawImage(1, 1, *pixel_image.image(), 0, 0, _sa->viewport()->width(), _sa->viewport()->height());
}

void
PixelTreeCanvas::constructPixelTree(void) {

  high_resolution_clock::time_point time_begin = high_resolution_clock::now();

  /// how many of each values after compression
  vlines = ceil((float)_nodeCount / approx_size);

  domain_red_arr.clear(); domain_red_arr.resize(vlines);

  /// get a root
  VisualNode* root = (*_na)[0];

  pixel_data = traverseTree(root);
  // pixel_data = traverseTreePostOrder(root);

  high_resolution_clock::time_point time_end = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(time_end - time_begin);
  std::cout << "Pixel Tree construction took: " << time_span.count() << " seconds." << std::endl;

}

void
PixelTreeCanvas::compressPixelTree(int value) {

  pixel_data.setCompression(value);
}

void
PixelTreeCanvas::compressDepthAnalysis
(std::vector< std::vector<unsigned int> >& da_data_compressed, int compression) {

  auto data_length = da_data.at(0).size();
  auto vlines = ceil((float)data_length / compression);

  // da_data_compressed = vector<vector<unsigned int> >(tree_depth, vector<unsigned int>(vlines));
  da_data_compressed.clear();
  da_data_compressed.resize(tree_depth);
  for (auto& v : da_data_compressed) {
    v.resize(vlines);
  }

  /// for every depth level
  for (unsigned depth = 0; depth < da_data.size(); depth++) {

    auto group_count = 0;
    auto group_value = 0;

    for (unsigned i = 0; i < data_length; i++) {

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

  for (unsigned i = 0; i < pixel_list.size(); i++) {
    group_count++;

    auto entry = _data.getEntry(pixel_list[i].node()->getIndex(*_na));
    auto value = (entry == nullptr) ? 0 : entry->node_time;
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

  for (unsigned i = 0; i < pixel_list.size(); i++) {
    group_count++;

    auto entry = _data.getEntry(pixel_list[i].node()->getIndex(*_na));
    auto value = (entry == nullptr) ? 0 : entry->domain;
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

  for (unsigned i = 0; i < var_decisions.size(); i++) {
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

    // if (label == "") { qDebug() << "empty label"; }

    /// 2. get variable name
    auto found = label.find(' ');

    string var = "";
    if (found!=std::string::npos)
      var = label.substr(0, found);
    // qDebug() << "var: " << var.c_str();

    /// 3. check if we already know the variable

    auto var_id = -1;

    for (unsigned i = 0; i < vars.size(); i++) {
      if (vars[i] == var) {
        var_id = i;
        break;
      }
    }

    if (var_id == -1) { /// no such variable 
      // qDebug() << "no such variable (" << var.c_str() << ")";
      vars.push_back(var);
      var_id = vars.size() - 1;
    } else {
      // qDebug() << "variable found (" << var.c_str() << ")";
    }

    /// rememeber decision variables
    var_decisions.push_back(var_id);
  }
}

void
PixelTreeCanvas::gatherNogoodData() {
  auto data_length = pixel_data.pixel_list.size();
  nogood_counts.resize(data_length);

  auto sid2nogood = _data.getNogoods();

  for (unsigned i = 0; i < data_length; i++) {
    auto node = pixel_data.pixel_list[i].node();
    auto it = sid2nogood.find(node->getIndex(*_na));
    if (it != sid2nogood.end()) {
      auto nogood = it->second;
      // qDebug() << "nogood: " << nogood.c_str();
      /// work out var length
      auto count = 0;
      auto pos = nogood.find(' ');
      while(pos != string::npos)
      {
          count++;
          pos = nogood.find(' ', pos + 1);
      }
      count -= 1; /// because in chuffed nogoods start "out_learnt (interpreted): ..."

      nogood_counts[i] = count;
    } else {
      nogood_counts[i] = 0; /// no nogood found
    }
  }
}

void
PixelTreeCanvas::compressNogoodData(int compression) {

  auto data_length = nogood_counts.size();
  auto vlines = ceil(data_length / compression);

  nogood_counts_compressed.clear();
  nogood_counts_compressed.resize(vlines);

  auto group_count = 0;
  auto group_value = 0.0f;

  for (unsigned i = 0; i < nogood_counts.size(); i++) {
    group_count++;

    /// TODO: ignore UNDET nodes (crashes otherwise)
    auto value = nogood_counts[i];
    group_value += value;

    if (group_count == compression) {
      unsigned int vline_id = i / compression;
      nogood_counts_compressed[vline_id] = group_value / group_count;
      group_count = 0; group_value = 0;
    }
  }

  /// deal with the last (not full) group
  if (group_count > 0) {
    unsigned int vline_id = data_length / compression;
    nogood_counts_compressed[vline_id] = group_value / group_count;
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

    pixelData.pixel_list.emplace_back(PixelItem(0, node, depth));

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

    pixelData.pixel_list.emplace_back(PixelItem(0, node, depth));

  }

  return pixelData;
}

void
PixelTreeCanvas::drawPixelTree(const PixelData& pixel_data) {

  auto xoff = _sa->horizontalScrollBar()->value(); // values should be in scaled pixels
  auto yoff = _sa->verticalScrollBar()->value();

  auto scale = pixel_image.scale();

  int img_width = pixel_image.width() / scale;

  const int compr = pixel_data.compression();
  const auto& pixel_list = pixel_data.pixel_list;

  // which pixel to start with:
  int start = xoff * compr;

  /// check for solutions first (we do not want solutions on top of nodes)
  for (auto pixel_id = start; pixel_id < int(pixel_list.size()); pixel_id++) {
    auto x = (pixel_id - start) / compr;
    if (x > img_width) break; /// out of image boundary

    auto pixelItem =  pixel_list[pixel_id];
    if (pixelItem.node()->getStatus() == SOLVED) {

      for (int depth = 0; depth < tree_depth; depth++) {
        int y = depth - yoff;
        if (y > img_width || y < 0) continue;
        pixel_image.drawPixel(x, y, qRgb(0, 255, 0));
      }
    }
  }

  std::vector<int> intensity_vec(tree_depth + 1, 0);

  for (auto pixel_id = start; pixel_id < int(pixel_list.size()); pixel_id++) {
    auto x = (pixel_id - start) / compr;
    if (x > img_width) break; /// out of image boundary

    /// work out y coordinate
    auto pixelItem =  pixel_list[pixel_id];
    auto depth = pixelItem.depth();

    intensity_vec.at(depth)++; // populate intensity vector

    bool is_vline_end = (pixel_id % compr == compr-1) || (pixel_id == int(pixel_list.size()-1));

    if (is_vline_end) {
      // draw from intensity vector:
      for (auto depth = 0; depth < int(intensity_vec.size()); depth++) {
        if (intensity_vec[depth] == 0) continue; /// no pixel at that depth

        auto y = depth - yoff;

        int value = 100 - 100 * static_cast<float>(intensity_vec[depth]) / compr;

        auto isSelected = pixelItem.isSelected();

        if (!isSelected)
          pixel_image.drawPixel(x, y, QColor::fromHsv(0, 0, value).rgba());
        else
          pixel_image.drawPixel(x, y, QColor::fromHsv(300, 255, 255).rgba());
      }

      std::fill(intensity_vec.begin(), intensity_vec.end(), 0);
    }

  }

  pixel_image.drawHorizontalLine(tree_depth - yoff);
  current_image_height += tree_depth + ceil(static_cast<float>(MARGIN) / pixel_image.scale());

}

/// Make sure no redundant work is done
void
PixelTreeCanvas::redrawAll() {

  pixel_image.clear();

  current_image_height = 0;

  auto scale = pixel_image.scale();
  int img_width = pixel_image.width();

  const auto& pixel_list = pixel_data.pixel_list;

  auto vlines = ceil((float)pixel_list.size() / pixel_data.compression());

  _sa->horizontalScrollBar()->setRange(0, vlines - img_width / scale + 20);

  /// TODO: ????
  this->resize(pixel_image.width(), pixel_image.height());

  drawPixelTree(pixel_data);

  /// All Histograms

  if (show_time_histogram) drawTimeHistogram();

  if (show_domain_histogram) drawDomainHistogram();

  // drawDomainReduction(image, leftmost_vline, rightmost_vline);

  // drawNodeRate(image, leftmost_vline, rightmost_vline);

  if (show_depth_analysis_histogram) drawDepthAnalysisData();

  if (show_decision_vars_histogram) drawVarData();

  if (show_bj_analysis_histogram) drawBjData();
  // drawNogoodData();

  pixel_image.update();

  _sa->verticalScrollBar()->setRange(0, current_image_height - _sa->viewport()->height() / scale);

  repaint();

}

void
PixelTreeCanvas::drawVarData() {

  /// TODO: add flush

  auto xoff = _sa->horizontalScrollBar()->value();
  auto yoff = _sa->verticalScrollBar()->value();

  auto scale = pixel_image.scale();
  int img_width = pixel_image.width() / scale;

  pixel_image.drawHorizontalLine(current_image_height - yoff);
  int zero_level = current_image_height + vars.size() - yoff;
  pixel_image.drawHorizontalLine(zero_level);

  for (auto vline = 0; vline < int(var_decisions_compressed.size()); vline++) {
    for (auto i = 0; i < int(var_decisions_compressed.at(vline).size()); i++) {

      auto var_id = var_decisions_compressed[vline][i];

      auto x = vline - xoff;
      auto y = zero_level - var_id;

      if (x > img_width) break;
      if (y > img_width || y < 0) continue;

      auto color_value = ceil(var_id * 255 / vars.size());

      pixel_image.drawPixel(x, y, QColor::fromHsv(color_value, 200, 255).rgba());

    }
  }

  current_image_height += vars.size() + ceil(static_cast<float>(MARGIN) / pixel_image.scale());
}

void
PixelTreeCanvas::drawNogoodData() {
  auto xoff = _sa->horizontalScrollBar()->value();
  auto yoff = _sa->verticalScrollBar()->value();

  auto scale = pixel_image.scale();
  int img_width = pixel_image.width() / scale;
  int img_height = pixel_image.height() / scale;


  auto max_value = *std::max(nogood_counts_compressed.begin(), nogood_counts_compressed.end());

  int zero_level = current_image_height + max_value - yoff;
  pixel_image.drawHorizontalLine(current_image_height - yoff);
  pixel_image.drawHorizontalLine(zero_level);

  for (auto vline = 0; vline < int(nogood_counts_compressed.size()); vline++) {
    auto value = nogood_counts_compressed[vline];

    auto x = vline - xoff;
    auto y = zero_level - value;

    if (x > img_width) break;
    if (y > img_height || y < 0) continue;

    /// TODO: normalize

    pixel_image.drawPixel(x, y, QColor::fromHsv(180, 200, 255).rgba());
  }

  

  current_image_height += max_value + ceil(static_cast<float>(MARGIN) / pixel_image.scale());
}

/// Draw time histogram underneath the pixel tree
void
PixelTreeCanvas::drawTimeHistogram() {

  drawHistogram(time_arr, qRgb(150, 150, 40));
}

void
PixelTreeCanvas::drawDomainHistogram() {
  drawHistogram(domain_arr, qRgb(150, 40, 150));
}

void
PixelTreeCanvas::drawDomainReduction() {
  // drawHistogram(2, domain_red_arr, l_vline, r_vline, qRgb(40, 150, 150));
}

void
PixelTreeCanvas::drawHistogram(vector<float>& data, int color) {

  auto xoff = _sa->horizontalScrollBar()->value();
  auto yoff = _sa->verticalScrollBar()->value();

  auto scale = pixel_image.scale();

  int img_width = pixel_image.width() / scale;
  int img_height = pixel_image.height() / scale;

  /// work out maximum value
  int max_value = *max_element(std::begin(data), std::end(data));

  if (max_value <= 0) { /// no data for this histogram
    // qDebug() << "(!) no data for this histogram";
    return;
  }


  float coeff = (static_cast<float>(HIST_HEIGHT) / scale) / (max_value + 1);

  pixel_image.drawHorizontalLine(current_image_height - yoff);

  auto zero_level = current_image_height + ceil((float)HIST_HEIGHT / scale) - yoff;

  pixel_image.drawHorizontalLine(zero_level);

  for (auto vline = 0; vline < int(data.size()); vline++) {
    auto val = floor(data[vline] * coeff);

    auto x = vline - xoff;
    auto y = zero_level - val;

    if (x < 0) continue;
    if (x > img_width) break; /// note: true (breaks) if x < 0
    if (y > img_height || y < 0) continue;

    pixel_image.drawPixel(x, y, color);


  }

  current_image_height += ceil(static_cast<float>(HIST_HEIGHT + MARGIN) / pixel_image.scale());
}

void
PixelTreeCanvas::drawNodeRate(unsigned l_vline, unsigned r_vline) {

  std::vector<float>& node_rate = _data.node_rate;
  std::vector<int>& nr_intervals = _data.nr_intervals;

  // int start_x = 0;

  float max_node_rate = *std::max_element(node_rate.begin(), node_rate.end());

  // float coeff = (float)HIST_HEIGHT / max_node_rate;

  int zero_level = tree_depth + HIST_HEIGHT;

  pixel_image.drawHorizontalLine(zero_level);

  for (unsigned i = 1; i < nr_intervals.size(); i++) {
    // float value = node_rate[i - 1] * coeff;
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
PixelTreeCanvas::drawBjData() {

  auto scale = pixel_image.scale();

  auto xoff = _sa->horizontalScrollBar()->value();
  auto yoff = _sa->verticalScrollBar()->value();

  int img_width = pixel_image.width() / scale;
  int img_height = pixel_image.height() / scale;

  auto max_value = bj_data.max_from;
  float coeff = (static_cast<float>(HIST_HEIGHT) / scale) / (max_value + 1);

  int zero_level = current_image_height + coeff * (max_value + 1) - yoff;

  pixel_image.drawHorizontalLine(current_image_height - yoff);
  pixel_image.drawHorizontalLine(zero_level);

  const auto compr = pixel_data.compression();
  const auto start = xoff;
  const auto end = std::ceil((float)vlines / compr);

  /// TODO(maxim): break if out of canvas' boundary
  for (auto vline = xoff; vline < end; ++vline) {

    /// TODO(maxim): get a list of nodes if compressed tree
    /// from `start` (including) to `start + compr` (not including)
    std::vector<BackjumpItem*> bj_items; /// items on this vline

    auto boundaries = getPixelBoundaries(vline, compr);
    auto first_id = boundaries.first;
    auto last_id = std::min(boundaries.second, (int)pixel_data.pixel_list.size());

    for (auto id = first_id; id < last_id; ++id ) {
      auto gid = pixel_data.pixel_list[id].node()->getIndex(*_na);
      auto bj_item = bj_data.bj_map.find(gid);
      if (bj_item != bj_data.bj_map.end()) {
        bj_items.push_back(&bj_item->second);
      }
    }

    
    if (bj_items.size() > 0) {

      auto average_from = std::accumulate(
        bj_items.begin(), bj_items.end(), 0,
        [](int sum, BackjumpItem* bj_item){
          return sum + bj_item->level_from;
        }
      ) / bj_items.size();

      auto average_to = std::accumulate(
        bj_items.begin(), bj_items.end(), 0,
        [](int sum, BackjumpItem* bj_item){
          return sum + bj_item->level_to;
        }
      ) / bj_items.size();

      auto average_skipped = std::accumulate(
        bj_items.begin(), bj_items.end(), 0,
        [](int sum, BackjumpItem* bj_item){
          return sum + bj_item->nodes_skipped;
        }
      ) / bj_items.size();


      auto x = vline - xoff;
      auto y_from = zero_level - coeff * average_from;
      auto y_to = zero_level - coeff * average_to;
      auto y_skipped = zero_level - coeff * average_skipped;

      pixel_image.drawPixel(x, y_from,  qRgb(170, 0, 0));
      pixel_image.drawPixel(x, y_to,  qRgb(0, 170, 0));
      pixel_image.drawPixel(x, y_skipped,  qRgb(0, 0, 170));
    }
  }
}

void
PixelTreeCanvas::drawDepthAnalysisData() {

  auto scale = pixel_image.scale();

  auto xoff = _sa->horizontalScrollBar()->value(); // values should be in scaled pixels
  auto yoff = _sa->verticalScrollBar()->value();

  int img_width = pixel_image.width() / scale;
  int img_height = pixel_image.height() / scale;

  auto max_value = [this](){
    auto data = this->da_data_compressed;
    auto max_vector = std::max_element(data.begin(), data.end(),
      [](vector<unsigned>& lhs, vector<unsigned>& rhs) {
        auto lhs_max =  *std::max_element(lhs.begin(), lhs.end());
        auto rhs_max =  *std::max_element(rhs.begin(), rhs.end());
        return lhs_max < rhs_max;
    });

    return *std::max_element(max_vector->begin(), max_vector->end());
  }();

  float coeff = (static_cast<float>(HIST_HEIGHT) / scale) / (max_value + 1);

  int zero_level = current_image_height + coeff * max_value - yoff;

  pixel_image.drawHorizontalLine(current_image_height - yoff - 1);
  pixel_image.drawHorizontalLine(zero_level);
  /// *** Actual Data ***
  /// for each depth level:
  for (auto depth = 0; depth < int(da_data_compressed.size()); depth++) {

    for (auto vline = xoff; vline < int(da_data_compressed[depth].size()); vline++) {

      auto value = da_data_compressed[depth][vline];
      if (value > max_value) max_value = value;
      // qDebug() << "depth: " << depth << " value: " << value;
      auto x = vline - xoff;
      auto y = zero_level - coeff * value;

      if (x < 0) continue;
      if (x > img_width) break; /// note: true (breaks) if x < 0
      if (y > img_height || y < 0) continue;

      int color_value = 200 - 200 * static_cast<float>(depth) / tree_depth;

      if (value != 0)
      pixel_image.drawPixel(x, y,  QColor::fromHsv(180, 180, color_value).rgba());
    }

  }

  current_image_height += coeff * max_value + ceil(static_cast<float>(MARGIN)/pixel_image.scale());


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
  compressNogoodData(value);
  redrawAll();
}

void
PixelTreeCanvas::sliderChanged(int) {
  /// calls redrawAll not more often than 60hz
  maybeCaller.call([this]() { redrawAll(); });
}

void
PixelTreeCanvas::toggleTimeHistogram(int state) {

  show_time_histogram = (state == Qt::Checked) ? true : false;

  redrawAll();
}

void
PixelTreeCanvas::toggleDomainsHistogram(int state) {

  show_domain_histogram = (state == Qt::Checked) ? true : false;

  redrawAll();
}

void
PixelTreeCanvas::toggleVarsHistogram(int state) {

  show_decision_vars_histogram = (state == Qt::Checked) ? true : false;

  redrawAll();
}

void
PixelTreeCanvas::toggleDepthAnalysisHistogram(int state) {

  show_depth_analysis_histogram = (state == Qt::Checked) ? true : false;

  redrawAll();
}

void
PixelTreeCanvas::mousePressEvent(QMouseEvent* event) {

  auto xoff = _sa->horizontalScrollBar()->value();
  auto vline = event->x() / pixel_image.scale() + xoff;

  selectNodesfromPT(vline);
  redrawAll();

}

void
PixelTreeCanvas::mouseMoveEvent(QMouseEvent* event) {

  // auto xoff = _sa->horizontalScrollBar()->value();
  // auto yoff = _sa->verticalScrollBar()->value();

  // auto vline = event->x() / pixel_image.scale() + xoff;
  // auto y = event->y() + yoff;


  // selectNodesfromPT(vline);
  /// TODO: find a way to get away without redrawing everything
  // pixel_image.drawMouseGuidelines(vline, 5);
  // redrawAll();

  // qDebug() << "mouse move: x: " << event->x() << " y: " << event->y();
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

  auto boundaries = getPixelBoundaries(vline, pixel_data.compression());
  auto start = boundaries.first;
  auto end = boundaries.second; /// not including

  Actions actions(_na, _tc);
  void (Actions::*apply)(VisualNode*);

  if (pixel_data.compression() == 1) {
    apply = &Actions::selectOne;
  } else {
    apply = &Actions::selectGroup;
    /// hide everything except root
    _tc.hideAll();
    (*_na)[0]->setHidden(false);
  }

  /// select nodes in interval [ start; end )

  // /// unset currently selected nodes
  for (auto& pixel : pixels_selected) {
    pixel->setSelected(false);
  }

  pixels_selected.clear();



  auto& pixel_list = pixel_data.pixel_list;

  for (auto id = start; id < end && id < pixel_list.size(); id++) {
    auto& pixelItem = pixel_list[id];
    auto node = pixelItem.node();
    (actions.*apply)(node);
    pixelItem.setSelected(true);
    pixels_selected.push_back(&pixelItem);
  }

  _tc.update();

}
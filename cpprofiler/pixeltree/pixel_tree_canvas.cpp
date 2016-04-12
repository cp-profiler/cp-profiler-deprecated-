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

#include "pixel_tree_canvas.hh"
#include <numeric>
#include <stack>
#include <utility>

#include "cpprofiler/analysis/backjumps.hh"

using namespace cpprofiler::pixeltree;
// using namespace std::chrono;
using std::vector; using std::list;

/// TODO(maxim): fix histograms going outside their boundaries
/// TODO(maxim): use correct height for the content
/// TODO(maxim): enable labels when hover over some histogram regions (like depth analysis)
/// TODO(maxim): find out why restarts highlight subtrees as if it was parallel execution
/// TODO(maxim): we could store some more informaiton about the execution (solver name etc)

/// ******** PIXEL_TREE_CANVAS ********

PixelTreeCanvas::PixelTreeCanvas(QWidget* parent, TreeCanvas& tc)
    : QWidget(parent), _tc(tc), _data(*tc.getExecution()->getData()), _na(tc.get_na()), depthAnalysis(tc)
{

  using cpprofiler::analysis::Backjumps;

  _sa = static_cast<QAbstractScrollArea*>(parentWidget());

  _nodeCount = tc.get_stats().allNodes();

  tree_depth = tc.get_stats().maxDepth;

  /// TODO(maxim): do I still need to do this? (check if tree depth is correct)
  // if (_tc->getData()->isRestarts()) {
  //   tree_depth++; /// consider the first, dummy node
  //   _nodeCount++;
  // }

  _sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  _sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

  setMouseTracking(true);

  connect (_sa->horizontalScrollBar(), SIGNAL(valueChanged (int)), this, SLOT(sliderChanged(int)));
  connect (_sa->verticalScrollBar(), SIGNAL(valueChanged (int)), this, SLOT(sliderChanged(int)));

  /// TODO(maxim): only run when asked for the first time?
  da_data = depthAnalysis.runMSL();

  Backjumps bj;
  bj_data = bj.findBackjumps((*_na)[0], *_na);

  constructPixelTree();
  compressPixelTree(1);
  compressDepthAnalysis(da_data_compressed, 1);
  compressTimeHistogram(time_arr, 1);
  getDomainDataCompressed(domain_arr, 1);
  gatherVarData();
  compressVarData(var_decisions_compressed, 1);
  gatherNogoodData();
  compressNogoodData(1);
  // resizeCanvas();
}

void
PixelTreeCanvas::paintEvent(QPaintEvent*) {
  if (pixel_image.image() == nullptr) return;

  QPainter painter(this);

  painter.drawImage(QPoint{0,0}, *pixel_image.image());
}

void
PixelTreeCanvas::constructPixelTree(void) {

  auto time_begin = high_resolution_clock::now();

  /// get a root
  auto root = (*_na)[0];

  pixel_data = traverseTree(root);
  // pixel_data = traverseTreePostOrder(root);

  auto time_end = high_resolution_clock::now();
  auto time_span = duration_cast<duration<double>>(time_end - time_begin);
  std::cout << "constructPixelTree: " << time_span.count() << " seconds." << std::endl;

}

/// This sets compression that will be used during the drawing
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
PixelTreeCanvas::getDomainDataCompressed(vector<float>& compressed, int compression) {

  auto time_begin = high_resolution_clock::now();

  auto& pixel_list = pixel_data.pixel_list;
  auto data_length = pixel_list.size();
  auto vlines = ceil(data_length / compression);

  compressed.clear();
  compressed.resize(vlines);

  auto group_count = 0;
  auto group_value = 0.0f;

  for (unsigned i = 0; i < pixel_list.size(); i++) {
    group_count++;

    auto entry = _data.getEntry(pixel_list[i].node()->getIndex(*_na));
    auto value = (entry == nullptr) ? 0 : entry->domain;
    group_value += value;

    if (group_count == compression) {
      auto vline_id = i / compression;
      compressed[vline_id] = group_value / group_count;
      group_count = 0; group_value = 0;
    }
  }

  /// deal with the last (not full) group
  if (group_count > 0) {
    auto vline_id = data_length / compression;
    compressed[vline_id] = group_value / group_count;
  }

  auto time_end = high_resolution_clock::now();
  auto time_span = duration_cast<duration<double>>(time_end - time_begin);
  // std::cout << "getDomainDataCompressed: " << time_span.count() << " seconds." << std::endl;
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

  auto time_begin = high_resolution_clock::now();

  auto data_length = pixel_data.pixel_list.size();
  var_decisions.reserve(data_length);

  for (auto& pixel : pixel_data.pixel_list) {
    auto label = _data.getLabel(pixel.node()->getIndex(*_na)); /// 1. get label

    /// 2. get variable name
    auto found = label.find(' ');

    string var = "";
    if (found!=std::string::npos)
      var = label.substr(0, found);

    /// 3. check if we already know the variable

    auto var_id = -1;

    for (unsigned i = 0; i < vars.size(); i++) {
      if (vars[i] == var) {
        var_id = i;
        break;
      }
    }

    if (var_id == -1) { /// no such variable
      vars.push_back(var);
      var_id = vars.size() - 1;
    } else {
    }

    /// rememeber decision variables
    var_decisions.push_back(var_id);
  }

  auto time_end = high_resolution_clock::now();
  auto time_span = duration_cast<duration<double>>(time_end - time_begin);
  std::cout << "gatherVarData: " << time_span.count() << " seconds." << std::endl;
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
  /// TODO(maxim): do I really need a stack here?
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

  const auto xoff = _sa->horizontalScrollBar()->value();
  const auto yoff = _sa->verticalScrollBar()->value();

  const auto img_width = pixel_image.width();

  const auto compr = pixel_data.compression();
  const auto& pixel_list = pixel_data.pixel_list;

  // which pixel to start with:
  const auto start = static_cast<unsigned>(xoff * compr);

  /// check for solutions first (should be on the background)
  for (auto pixel_id = start; pixel_id < pixel_list.size(); pixel_id++) {
    unsigned x = (pixel_id - start) / compr;
    if (x > img_width) break; /// out of image boundary

    auto pixelItem =  pixel_list[pixel_id];
    if (pixelItem.node()->getStatus() == SOLVED) {

      for (unsigned depth = 0; depth < tree_depth; depth++) {
        int y = depth - yoff;
        if (static_cast<unsigned>(y) > img_width || y < 0) continue;
        pixel_image.drawPixel(x, y, qRgb(0, 255, 0));
      }
    }
  }

  std::vector<int> intensity_vec(tree_depth + 1, 0);
  std::vector<bool> selected_on_level(tree_depth + 1, false);

  for (auto pixel_id = start; pixel_id < pixel_list.size(); pixel_id++) {
    auto x = (pixel_id - start) / compr;
    if (x > img_width) break; /// out of image boundary

    /// work out y coordinate
    auto pixelItem =  pixel_list[pixel_id];
    auto depth = pixelItem.depth();

    intensity_vec.at(depth)++; // populate intensity vector
    if (pixelItem.isSelected()) { selected_on_level[depth] = true; }

    bool is_vline_end = ( (pixel_id % compr) == (compr-1) ) || (pixel_id == pixel_list.size()-1);

    if (is_vline_end) {
      // draw from intensity vector:
      for (auto depth = 0; depth < int(intensity_vec.size()); depth++) {
        if (intensity_vec[depth] == 0) continue; /// no pixel at that depth

        auto y = depth - yoff;

        int value = 100 - 100 * static_cast<float>(intensity_vec[depth]) / compr;

        if (!selected_on_level[depth])
          pixel_image.drawPixel(x, y, QColor::fromHsv(0, 0, value).rgba());
        else {
          pixel_image.drawPixel(x, y, QColor::fromHsv(300, 255, 255).rgba());
        }
      }

      std::fill(intensity_vec.begin(), intensity_vec.end(), 0);
      std::fill(selected_on_level.begin(), selected_on_level.end(), false);
    }

  }

  pixel_image.drawHorizontalLine(tree_depth - yoff, PixelImage::PIXEL_COLOR::LIGTH_GRAY);

  current_image_height += tree_depth + MARGIN + 1;

}

/// Make sure no redundant work is done
void
PixelTreeCanvas::redrawAll() {

  pixel_image.clear();

  /// TODO(maxim): this should probably also be in the pixelImage class
  current_image_height = 0;

  const auto& pixel_list = pixel_data.pixel_list;

  auto vlines = ceil((float)pixel_list.size() / pixel_data.compression());

  _sa->horizontalScrollBar()->setRange(0, vlines - pixel_image.width() + 20);
  _sa->verticalScrollBar()->setRange(0, current_image_height - _sa->viewport()->height() / pixel_image.pixel_height());

  drawPixelTree(pixel_data);

  /// All Histograms

  if (show_time_histogram) drawTimeHistogram();

  if (show_domain_histogram) drawDomainHistogram();

  // drawDomainReduction(image, leftmost_vline, rightmost_vline);

  // drawNodeRate(image, leftmost_vline, rightmost_vline);

  if (show_depth_analysis_histogram) drawDepthAnalysisData();
  if (show_depth_analysis_histogram) drawDepthAnalysisData2();

  if (show_decision_vars_histogram) drawVarData();

  if (show_bj_analysis_histogram) drawBjData();
  // drawNogoodData();

  pixel_image.update();

  repaint();

}

void
PixelTreeCanvas::drawVarData() {

  /// TODO: add flush

  const auto xoff = _sa->horizontalScrollBar()->value();
  const auto yoff = _sa->verticalScrollBar()->value();

  const int zero_level = current_image_height + vars.size() - yoff;
  pixel_image.drawHorizontalLine(current_image_height - yoff, PixelImage::PIXEL_COLOR::LIGTH_GRAY);
  pixel_image.drawHorizontalLine(zero_level, PixelImage::PIXEL_COLOR::LIGTH_GRAY);

  for (unsigned vline = 0; vline < var_decisions_compressed.size(); vline++) {
    for (unsigned i = 0; i < var_decisions_compressed.at(vline).size(); i++) {

      const auto var_id = var_decisions_compressed[vline][i];

      /// cast to `int` as to see if the value < 0
      const auto x = static_cast<int>(vline) - xoff;
      const auto y = static_cast<int>(zero_level) - var_id;

      if (static_cast<unsigned>(x) > pixel_image.width()) break;
      if (static_cast<unsigned>(y) > pixel_image.width() || y < 0) continue;

      const auto color_value = ceil(var_id * 255 / vars.size());

      pixel_image.drawPixel(x, y, QColor::fromHsv(color_value, 200, 255).rgba());

    }
  }

  current_image_height += vars.size() + MARGIN;
}

void
PixelTreeCanvas::drawNogoodData() {
  const auto xoff = _sa->horizontalScrollBar()->value();
  const auto yoff = _sa->verticalScrollBar()->value();

  const auto img_height = pixel_image.height();

  const auto max_value = *std::max(nogood_counts_compressed.begin(), nogood_counts_compressed.end());

  const int zero_level = current_image_height + max_value - yoff;
  pixel_image.drawHorizontalLine(current_image_height - yoff, PixelImage::PIXEL_COLOR::LIGTH_GRAY);
  pixel_image.drawHorizontalLine(zero_level, PixelImage::PIXEL_COLOR::LIGTH_GRAY);

  for (unsigned vline = 0; vline < nogood_counts_compressed.size(); vline++) {
    const auto value = nogood_counts_compressed[vline];

    /// cast to `int` as to see if the value < 0
    const auto x = static_cast<int>(vline) - xoff;
    const auto y = static_cast<int>(zero_level) - value;

    if (static_cast<unsigned>(x) > pixel_image.width()) break;
    if (static_cast<unsigned>(y) > img_height || y < 0) continue;

    /// TODO: normalize

    pixel_image.drawPixel(x, y, QColor::fromHsv(180, 200, 255).rgba());
  }



  current_image_height += max_value + MARGIN;
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

  auto img_height = pixel_image.height();

  /// work out maximum value
  int max_value = *max_element(std::begin(data), std::end(data));

  if (max_value <= 0) { /// no data for this histogram
    // qDebug() << "(!) no data for this histogram";
    return;
  }


  float coeff = static_cast<float>(HIST_HEIGHT) / (max_value + 1);

  pixel_image.drawHorizontalLine(current_image_height - yoff, PixelImage::PIXEL_COLOR::LIGTH_GRAY);

  auto zero_level = current_image_height + (float)HIST_HEIGHT  - yoff;

  pixel_image.drawHorizontalLine(zero_level, PixelImage::PIXEL_COLOR::LIGTH_GRAY);

  for (unsigned vline = 0; vline < data.size(); vline++) {
    auto val = floor(data[vline] * coeff);

    auto x = static_cast<int>(vline) - xoff;
    auto y = static_cast<int>(zero_level) - val;

    if (x < 0) continue;
    if (static_cast<unsigned>(x) > pixel_image.width()) break; /// note: true (breaks) if x < 0
    if (static_cast<unsigned>(y) > img_height || y < 0) continue;

    pixel_image.drawPixel(x, y, color);


  }

  current_image_height += HIST_HEIGHT + MARGIN;
}

void
PixelTreeCanvas::drawNodeRate(unsigned l_vline, unsigned r_vline) {

  // std::vector<float>& node_rate = _data.node_rate;
  std::vector<int>& nr_intervals = _data.nr_intervals;

  // int start_x = 0;

  // float max_node_rate = *std::max_element(node_rate.begin(), node_rate.end());

  // float coeff = (float)HIST_HEIGHT / max_node_rate;

  int zero_level = tree_depth + HIST_HEIGHT;

  pixel_image.drawHorizontalLine(zero_level, PixelImage::PIXEL_COLOR::LIGTH_GRAY);

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

static std::pair<int, int>
getPixelBoundaries(int vline_begin, int vline_end, int compression) {
  return std::make_pair(vline_begin * compression, (vline_end + 1) * compression);
}

static std::pair<int, int>
getPixelBoundaries(int vline, int compression) {
  return getPixelBoundaries(vline, vline, compression);
}

void
PixelTreeCanvas::drawBjData() {

  auto xoff = _sa->horizontalScrollBar()->value();
  auto yoff = _sa->verticalScrollBar()->value();

  /// TODO(maxim): check boundaries for this histogram
  // auto img_width = pixel_image.width();
  // auto img_height = pixel_image.height();

  auto max_value = bj_data.max_from;
  float coeff = static_cast<float>(HIST_HEIGHT) / (max_value + 1);

  int zero_level = current_image_height + coeff * (max_value + 1) - yoff;

  pixel_image.drawHorizontalLine(current_image_height - yoff, PixelImage::PIXEL_COLOR::LIGTH_GRAY);
  pixel_image.drawHorizontalLine(zero_level, PixelImage::PIXEL_COLOR::LIGTH_GRAY);

  const auto compr = pixel_data.compression();
  const auto end = std::ceil((float)_nodeCount / compr);

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
      auto y_from = current_image_height - yoff + ceil(coeff * average_from);
      auto y_to = current_image_height - yoff + ceil(coeff * average_to);

      for (int y = y_from; y >= y_to; --y) {
        if (y > y_from - average_skipped) {
          pixel_image.drawPixel(x, y,  qRgb(170, 0, 0));
        } else {
          pixel_image.drawPixel(x, y,  qRgb(170, 170, 0));
        }
      }

    }
  }
}

void
PixelTreeCanvas::drawDepthAnalysisData() {

  auto xoff = _sa->horizontalScrollBar()->value(); // values should be in scaled pixels
  auto yoff = _sa->verticalScrollBar()->value();

  /// only calculate this once
  if (da_data_max == 0) {
    da_data_max = [this](){
      auto data = this->da_data_compressed;
      auto max_vector = std::max_element(data.begin(), data.end(),
        [](vector<unsigned>& lhs, vector<unsigned>& rhs) {
          auto lhs_max =  *std::max_element(lhs.begin(), lhs.end());
          auto rhs_max =  *std::max_element(rhs.begin(), rhs.end());
          return lhs_max < rhs_max;
      });

      return *std::max_element(max_vector->begin(), max_vector->end());
    }();
  }

  const int max_value = da_data_max;

  float coeff = max_value > HIST_HEIGHT ?
                      static_cast<float>(HIST_HEIGHT) / (max_value + 1) : 1;

  const int zero_level = current_image_height + HIST_HEIGHT - yoff;

  pixel_image.drawHorizontalLine(current_image_height - yoff - 1, PixelImage::PIXEL_COLOR::LIGTH_GRAY);
  pixel_image.drawHorizontalLine(zero_level - 1, PixelImage::PIXEL_COLOR::LIGTH_GRAY);
  /// *** Actual Data ***
  /// for each depth level:
  for (auto depth = 0; depth < int(da_data_compressed.size()); depth++) {

    for (auto vline = xoff; vline < int(da_data_compressed[depth].size()); vline++) {

      auto value = da_data_compressed[depth][vline];

      auto x = static_cast<int>(vline) - xoff;
      auto y = static_cast<int>(zero_level) - coeff * value;

      if (static_cast<unsigned>(x) > pixel_image.width()) break; /// note: true (breaks) if x < 0
      if (static_cast<unsigned>(y) > pixel_image.height() || y < 0) continue;

      int color_value = 200 - 200 * static_cast<float>(depth) / tree_depth;

      if (value != 0)
      pixel_image.drawPixel(x, y,  QColor::fromHsv(180, 180, color_value).rgba());
    }

  }

  current_image_height += HIST_HEIGHT + MARGIN;

}

void
PixelTreeCanvas::drawDepthAnalysisData2() {

  auto xoff = _sa->horizontalScrollBar()->value(); // values should be in scaled pixels
  auto yoff = _sa->verticalScrollBar()->value();

  /// only calculate this once
  if (da_data_max == 0) {
    da_data_max = [this](){
      auto data = this->da_data_compressed;
      auto max_vector = std::max_element(data.begin(), data.end(),
        [](vector<unsigned>& lhs, vector<unsigned>& rhs) {
          auto lhs_max =  *std::max_element(lhs.begin(), lhs.end());
          auto rhs_max =  *std::max_element(rhs.begin(), rhs.end());
          return lhs_max < rhs_max;
      });

      return *std::max_element(max_vector->begin(), max_vector->end());
    }();
  }

  const int max_value = da_data_max;
  const int max_depth = da_data_compressed.size();

  const float coeff = static_cast<float>(255) / (max_value + 1);

  int zero_level = current_image_height + max_depth - yoff;

  pixel_image.drawHorizontalLine(current_image_height - 1 - yoff, PixelImage::PIXEL_COLOR::LIGTH_GRAY);
  pixel_image.drawHorizontalLine(zero_level - 1, PixelImage::PIXEL_COLOR::LIGTH_GRAY);
  /// *** Actual Data ***
  /// for each depth level:
  for (auto depth = 0; depth < max_depth; depth++) {

    for (auto vline = xoff; vline < int(da_data_compressed[depth].size()); vline++) {

      auto value = da_data_compressed[depth][vline];

      auto x = static_cast<int>(vline) - xoff;
      auto y = static_cast<int>(current_image_height) + depth - yoff;
      int color_value = 255 - coeff * value;

      if (static_cast<unsigned>(x) > pixel_image.width()) break; /// note: true (breaks) if x < 0
      if (static_cast<unsigned>(y) > pixel_image.height() || y < 0) continue;

      // if (value != 0) {
        pixel_image.drawPixel(x, y,  QColor::fromHsv(0, 0, color_value).rgba());
      // }
    }

  }

  current_image_height += max_depth + MARGIN;

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

  auto sa_width = _sa->viewport()->width();
  auto sa_height =  _sa->viewport()->height();
  pixel_image.resize(sa_width, sa_height);
  this->resize(sa_width, sa_height);

  _sa->horizontalScrollBar()->setPageStep(_sa->viewport()->width());
  _sa->horizontalScrollBar()->setSingleStep(100); /// the value is arbitrary
  redrawAll();
}

void
PixelTreeCanvas::compressionChanged(int value) {
  compressPixelTree(value);
  compressDepthAnalysis(da_data_compressed, value);
  compressTimeHistogram(time_arr, value);
  getDomainDataCompressed(domain_arr, value);
  compressVarData(var_decisions_compressed, value);
  compressNogoodData(value);
  redrawAll();
}

/// TODO(maxim): try to use lambda in the connect statement itself
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
PixelTreeCanvas::toggleBjHistogram(int state) {

  show_bj_analysis_histogram = (state == Qt::Checked) ? true : false;

  redrawAll();
}

void
PixelTreeCanvas::mousePressEvent(QMouseEvent* event) {

  auto xoff = _sa->horizontalScrollBar()->value();
  mouse_pressed_vline = event->x() / pixel_image.pixel_width() + xoff;

  mouse_pressed = true;

}

void
PixelTreeCanvas::mouseReleaseEvent(QMouseEvent* event) {

  if (mouse_pressed) {

    auto xoff = _sa->horizontalScrollBar()->value();
    auto mouse_released_vline = event->x() / pixel_image.pixel_width() + xoff;

    if (mouse_pressed_vline > mouse_released_vline) {
      std::swap(mouse_pressed_vline, mouse_released_vline);
    }

    selectNodesfromPT(mouse_pressed_vline, mouse_released_vline);

    redrawAll();

    /// Note(maxim): Faking the mouseMoveEvent to fix the pixel highlighting
    /// disappearing after mouse release (is there a cleaner way?)
    /// Have to reset these, or the the mouse over action will be bypassed:
    mouse_guide_x = -1; mouse_guide_y = -1;
    QMouseEvent mouse_event{QEvent::MouseMove, QPoint(event->x(), event->y()),
        Qt::NoButton, Qt::NoButton, Qt::NoModifier};
    qApp->sendEvent(this, &mouse_event);

  }
  mouse_pressed = false;
}

void
PixelTreeCanvas::mouseMoveEvent(QMouseEvent* event) {

  const auto xoff = _sa->horizontalScrollBar()->value();

  const auto x = event->x() / pixel_image.pixel_width();
  const auto y = event->y() / pixel_image.pixel_height();

  if (x != mouse_guide_x || y != mouse_guide_y) {
    mouse_guide_x = x;
    mouse_guide_y = y;

    const auto vline = x + xoff;

    /// calls redrawAll not more often than 60hz
    maybeCaller.call([this, x, y, vline]() {

      pixel_image.drawMouseGuidelines(x, y);
      pixel_image.update();
      repaint();

      highlightOnOriginalTree(vline);

    });

  }

}

namespace detail {

  static void unselectPixels(std::vector<PixelItem*>& pixels_selected) {
    for (auto& pixel : pixels_selected) {
      pixel->setSelected(false);
    }
    pixels_selected.clear();
  }

  static void unhighlightNodes(NodeAllocator& na, std::vector<PixelItem*>& pixels_mouse_over) {
    for (auto& pixel : pixels_mouse_over) {
      pixel->unsetHovered(na);
    }
    pixels_mouse_over.clear();
  }

}


void
PixelTreeCanvas::selectNodesfromPT(int vline_begin, int vline_end) {

  auto boundaries = getPixelBoundaries(vline_begin, vline_end, pixel_data.compression());
  unsigned start = boundaries.first;
  unsigned end = boundaries.second; /// not including

  auto selectOne = [this](VisualNode* node) {

    _tc.unhideNode(node);

    _tc.setCurrentNode(node);
    _tc.centerCurrentNode();
  };

  auto selectGroup = [this](VisualNode* node) {
    _tc.unhideNode(node);
  };

  std::function<void(VisualNode*)> apply = selectOne;

  if (pixel_data.compression() == 1 && (vline_begin == vline_end)) {
    apply = selectOne;
  } else {
    apply = selectGroup;
    /// hide everything except root
    _tc.hideAll();
    (*_na)[0]->setHidden(false);
  }

  /// unset currently selected nodes
  detail::unselectPixels(pixels_selected);

  /// select nodes in interval [ start; end )
  auto& pixel_list = pixel_data.pixel_list;

  for (unsigned id = start; id < end && id < pixel_list.size(); ++id) {
    auto& pixelItem = pixel_list[id];
    auto node = pixelItem.node();

    apply(node);

    /// TODO(maxim): Do I really want to set selected when selecting > 1 as well??

    pixelItem.setSelected(true);
    pixels_selected.push_back(&pixelItem);
  }

  _tc.update();

}

void
PixelTreeCanvas::highlightOnOriginalTree(int vline) {
  auto boundaries = getPixelBoundaries(vline, pixel_data.compression());
  unsigned start = boundaries.first;
  unsigned end = boundaries.second; /// not including

  /// unhighlight previously highlighted
  detail::unhighlightNodes(*_na, pixels_mouse_over);

  auto& pixel_list = pixel_data.pixel_list;

  for (unsigned id = start; id < end && id < pixel_list.size(); ++id) {
    auto& pixelItem = pixel_list[id];

    /// hightlight itself if visible or the first visible parent otherwise
    pixelItem.setHovered(*_na);
    pixels_mouse_over.push_back(&pixelItem);
  }

  _tc.update();
}


PixelItem&
PixelTreeCanvas::gid2PixelItem(int gid) {
  auto& pixel_list = pixel_data.pixel_list;

  for (auto& pixelItem : pixel_list) {
    if (pixelItem.gid(*_na) == gid) return pixelItem;
  }

  assert(false);
}

void
PixelTreeCanvas::setPixelSelected(int gid) {

  /// unset currently selected nodes
  detail::unselectPixels(pixels_selected);

  auto& pixelItem = gid2PixelItem(gid);
  pixelItem.setSelected(true);
  pixels_selected.push_back(&pixelItem);

  /// TODO(maxim): confirm that I need to redraw everything
  redrawAll();
}


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

#ifndef CPPROFILER_PIXELTREE_CANVAS_HH
#define CPPROFILER_PIXELTREE_CANVAS_HH

#include <QWidget>
#include <vector>
#include <string>

#include "cpprofiler/analysis/depth_analysis.hh"
#include "cpprofiler/analysis/backjumps.hh"
#include "pixel_data.hh"

class Data;
class TreeCanvas;

template<class T> class NodeAllocatorBase;
class VisualNode;

class QAbstractScrollArea;
class QScrollBar;
class PixelItem;
class SpaceNode;

typedef NodeAllocatorBase<VisualNode> NodeAllocator;

namespace cpprofiler { namespace analysis {
  class BackjumpItem;
}}

namespace cpprofiler { namespace pixeltree {

using cpprofiler::analysis::DepthAnalysis;
using cpprofiler::analysis::BackjumpItem;
using cpprofiler::analysis::BackjumpData;

class PixelTreeCanvas : public QWidget {
  Q_OBJECT

private:
  TreeCanvas&     _tc;
  Data&           _data;
  NodeAllocator*  _na;
  QPixmap         pixmap;

  QAbstractScrollArea*  _sa;
  QScrollBar*           _vScrollBar;

  /// Constants for a particular execution
  unsigned _nodeCount;

  /// Pixel Tree settings (changed through GUI)

  unsigned approx_size = 1; // how many nodes per vertical line

  unsigned   vline_idx;  // same as x when _step = 1

  /// Stuff specific for a particular pixel tree
  unsigned vlines; /// width of pixel tree
  int tree_depth;

  std::vector<float> time_arr; // time for each vline
  std::vector<float> domain_arr; // domain for each vline
  std::vector<float> domain_red_arr; /// domain reduction for each vline

  std::vector<std::string> vars;

  std::vector<PixelItem*> pixels_selected; // to know which pixels to deselect

  std::vector<int> var_decisions;
  std::vector<std::vector<int>> var_decisions_compressed;

  std::vector<int> nogood_counts;
  std::vector<int> nogood_counts_compressed;

  /// Depth analysis data
  DepthAnalysis depthAnalysis;
  std::vector< std::vector<unsigned> > da_data;
  std::vector< std::vector<unsigned> > da_data_compressed;

  /// Backjumps analysis data
  BackjumpData bj_data;

  PixelImage pixel_image;

  PixelData pixel_data;

  bool show_time_histogram = true;
  bool show_domain_histogram = true;
  bool show_decision_vars_histogram = true;
  bool show_depth_analysis_histogram = true;
  bool show_bj_analysis_histogram = true;

  unsigned current_image_height; /// in 'squares'

  MaybeCaller maybeCaller;

public:

  static const int HIST_HEIGHT = 80;
  static const int MARGIN = 30;

private:

  void drawPixelTree(const PixelData& pixel_data);

  /// Decision variables
  void gatherVarData();
  void compressVarData(std::vector<std::vector<int> >&, int value);

  void gatherNogoodData();
  void compressNogoodData(int value);
  void drawNogoodData();

  void constructPixelTree(); /// Initial Search Tree traversal
  /// Apply compression (to get vlineData)
  void compressPixelTree(int value);
  void compressDepthAnalysis(std::vector< std::vector<unsigned int> >& data, int value);
  void compressTimeHistogram(std::vector<float>&, int value);
  void compressDomainHistogram(std::vector<float>&, int value);
  PixelData traverseTree(VisualNode* node);
  PixelData traverseTreePostOrder(VisualNode* node);

  void redrawAll();
  void drawHistogram(std::vector<float>& data, int color);

  void drawSolutionLine();

  /// Histograms
  void drawTimeHistogram();
  void drawDomainHistogram();
  void drawDomainReduction();
  void drawDepthAnalysisData();
  void drawBjData();

  
  void drawVarData();

  /// Node Rate
  void drawNodeRate(unsigned leftmost_vline, unsigned rightmost_vline);

  /// select nodes that correspond to selected vline in pixel tree
  void selectNodesfromPT(unsigned vline);
  

public:
  PixelTreeCanvas(QWidget* parent, TreeCanvas& tc);

protected:
  void paintEvent(QPaintEvent* event);
  void mousePressEvent(QMouseEvent* me);
  void mouseMoveEvent(QMouseEvent* me);

public Q_SLOTS:
  void scaleUp(void);
  void scaleDown(void);
  void compressionChanged(int value);
  void resizeCanvas(void);
  void sliderChanged(int value);

  void toggleTimeHistogram(int state);
  void toggleDomainsHistogram(int state);
  void toggleVarsHistogram(int state);
  void toggleDepthAnalysisHistogram(int state);
};

}}


#endif
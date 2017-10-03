
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
#include <QDebug>

#include "cpprofiler/analysis/depth_analysis.hh"
#include "cpprofiler/analysis/backjumps.hh"
#include "pixel_data.hh"
#include "pixelImage.hh"
#include "maybeCaller.hh"

class Data;
class TreeCanvas;

class VisualNode;

class QAbstractScrollArea;
class QScrollBar;
class SpaceNode;

namespace cpprofiler {
namespace analysis {
struct BackjumpItem;
}
}

namespace cpprofiler {
namespace pixeltree {

using cpprofiler::analysis::DepthAnalysis;
using cpprofiler::analysis::BackjumpItem;
using cpprofiler::analysis::BackjumpData;

class PixelItem;
class InfoPanel;

class HistogramDesc {
public:
  int var_begin;
  int var_end;

  int time_begin;
  int time_end;

  void infoAtXY(int vline, int y) {
    qDebug() << "x: " << vline << " y: " << y;
    if (y > var_begin && y <= var_end) {
      qDebug() << "INSIDE!";

      // const std::vector<int>& var_ids = var_decisions_compressed[vline];

      // qDebug() << var_ids[var_end - y];
    }
  }
};

class PixelTreeCanvas : public QWidget {
  Q_OBJECT

  struct PixelTreeState {
    bool show_objective = true;
    bool show_time_histogram = false;
    bool show_domain_histogram = false;
    bool show_decision_vars_histogram = false;
    bool show_depth_analysis_histogram = false;
    bool show_bj_analysis_histogram = false;

    /// mouse guidelines
    int mouse_guide_x = 0;
    int mouse_guide_y = 0;

    /// handling multiple selection on a pixel tree
    bool mouse_pressed = false;
    int mousePressedVline;

    // how many nodes per vertical line
    int approximation = 1;
  };

 private:
  TreeCanvas& _tc;
  Data& _data;
  NodeAllocator& _na;
  QPixmap pixmap;

  HistogramDesc hisogramDesc;
  InfoPanel& infoPanel;

  QAbstractScrollArea* _sa;

  /// Constants for a particular execution
  int _nodeCount; 

  /// Stuff specific for a particular pixel tree
  int tree_depth;

  std::vector<float> time_arr;        // time for each vline
  std::vector<float> domain_arr;      // domain for each vline
  std::vector<float> domain_red_arr;  /// domain reduction for each vline
  std::vector<float> objective_arr;

  std::vector<std::string> vars;

  // to know which pixels to deselect
  std::vector<PixelItem*> pixels_selected;

  // to know which pixels to unhighlight
  std::vector<PixelItem*> pixels_mouse_over;

  std::vector<int> var_decisions;
  std::vector<std::vector<int>> var_decisions_compressed;

  std::vector<int> nogood_counts;
  std::vector<int> nogood_counts_compressed;

  /// Depth analysis data
  DepthAnalysis depthAnalysis;
  int da_data_max = 0;  // to be assigned
  std::vector<std::vector<unsigned>> da_data;
  std::vector<std::vector<unsigned>> da_data_compressed;

  PixelTreeState m_State;

  /// Backjumps analysis data
  BackjumpData bj_data;

  PixelImage pixel_image;

  PixelData pixel_data;

  int current_image_height;  /// in 'squares'

  MaybeCaller maybeCaller;

 public:
  static constexpr int HIST_HEIGHT = 10;  // in fake pixels
  static constexpr int MARGIN = 2;       // in fake pixels

 private:
  void drawPixelTree(const PixelData& pixel_data);

  /// Decision variables
  void gatherVarData();
  void compressVarData(std::vector<std::vector<int>>&, int value);

  void gatherNogoodData();
  void compressNogoodData(int value);
  void drawNogoodData();

  void constructPixelTree(bool post_order = false);  /// Initial Search Tree traversal
  /// Apply compression (to get vlineData)
  void compressPixelTree(int value);
  void compressDepthAnalysis(std::vector<std::vector<unsigned int>>& data,
                             int value);
  void compressTimeHistogram(std::vector<float>&, int value);
  void getDomainDataCompressed(std::vector<float>&, int value);

  void gatherObjectiveData();
  void compressObjectiveHistogram(std::vector<float>&, int value);
  PixelData traverseTree(VisualNode* node);
  PixelData traverseTreePostOrder(VisualNode* node);

  /// Re-calculate the entire tree (i.e. with different traversal type)
  void reset(bool post_order, int compression);

  void redrawAll();
  void drawHistogram(const std::vector<float>& data, int color);

  void drawSolutionLine();

  /// Histograms
  void drawObjective();
  void drawTimeHistogram();
  void drawDomainHistogram();
  void drawDomainReduction();
  void drawDepthAnalysisData();

  // TODO(maxim): better name
  void drawDepthAnalysisData2();
  void drawBjData();

  void drawVarData();

  /// Node Rate
  void drawNodeRate(unsigned leftmost_vline, unsigned rightmost_vline);

  /// select nodes that correspond to selected vline(s) in pixel tree
  void selectNodesfromPT(int vline_begin, int vline_end);
  /// highlight nodes on mouse over pixel tree
  void highlightOnOriginalTree(int vline);

  PixelItem& gid2PixelItem(int gid);

 public:
  PixelTreeCanvas(QWidget* parent, TreeCanvas& tc, InfoPanel& ip);

 protected:
  void paintEvent(QPaintEvent* event);
  void mousePressEvent(QMouseEvent* me);
  void mouseReleaseEvent(QMouseEvent* me);
  void mouseMoveEvent(QMouseEvent* me);

 public Q_SLOTS:
  void scaleUp(void);
  void scaleDown(void);
  void compressionChanged(int value);
  void resizeCanvas(void);
  void sliderChanged(int value);

  void changeTraversalType(const QString& type);

  void toggleTimeHistogram(int state);
  void toggleDomainsHistogram(int state);
  void toggleVarsHistogram(int state);
  void toggleDepthAnalysisHistogram(int state);
  void toggleBjHistogram(int state);
  void setPixelSelected(int gid);
};
}
}

#endif

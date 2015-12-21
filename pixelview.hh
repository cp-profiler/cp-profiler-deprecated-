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

#ifndef PIXEL_VIEW_HH
#define PIXEL_VIEW_HH

#include "treecanvas.hh"
#include "depth_analysis.hh"
#include "maybeCaller.hh"
#include <list>
#include <vector>

#include "pixelImage.hh"

using std::vector; using std::list;

/// ***********************************

class PixelItem {
private:
  int   _idx;
  int   _depth;
  VisualNode* _node;
  bool  _selected;

public:
  PixelItem(int idx, VisualNode* node, int depth)
  : _idx(idx), _depth(depth), _node(node), _selected(false) {};
  inline int idx() const { return _idx; }
  inline int depth() const { return _depth; }
  inline VisualNode* node() { return _node; }
  inline bool isSelected() { return _selected; }
  void setSelected(bool value) {
    _node->setSelected(value);
    _selected = value;
  }
};

class PixelData {

private:
  int compression_;

public:
  PixelData() {}

  explicit PixelData(unsigned int node_count): compression_(1) {
    pixel_list.reserve(node_count);
  }

  void setCompression(int compression) {
    compression_ = compression;
  }

  int compression() const { return compression_; }

  vector<PixelItem> pixel_list;

};


/// ******** PIXEL_TREE_CANVAS ********


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
  unsigned tree_depth;

  vector<float> time_arr; // time for each vline
  vector<float> domain_arr; // domain for each vline
  vector<float> domain_red_arr; /// domain reduction for each vline

  vector<string> vars;

  std::vector<PixelItem*> pixels_selected; // to know which pixels to deselect

  vector<int> var_decisions;
  vector<vector<int>> var_decisions_compressed;

  vector<int> nogood_counts;
  vector<int> nogood_counts_compressed;

  /// Depth analysis data
  DepthAnalysis depthAnalysis;
  std::vector< std::vector<unsigned> > da_data;
  std::vector< std::vector<unsigned> > da_data_compressed;

  PixelImage pixel_image;

  PixelData pixel_data;

  bool show_time_histogram = true;
  bool show_domain_histogram = true;
  bool show_decision_vars_histogram = true;
  bool show_depth_analysis_histogram = true;

  unsigned current_image_height; /// in 'squares'

  MaybeCaller maybeCaller;

public:

  static const int HIST_HEIGHT = 80;
  static const int MARGIN = 30;

private:

  void drawPixelTree(const PixelData& pixel_data);

  /// Decision variables
  void gatherVarData();
  void compressVarData(vector<vector<int> >&, int value);

  void gatherNogoodData();
  void compressNogoodData(int value);
  void drawNogoodData();

  void constructPixelTree(); /// Initial Search Tree traversal
  /// Apply compression (to get vlineData)
  void compressPixelTree(int value);
  void compressDepthAnalysis(std::vector< std::vector<unsigned int> >& data, int value);
  void compressTimeHistogram(vector<float>&, int value);
  void compressDomainHistogram(vector<float>&, int value);
  PixelData traverseTree(VisualNode* node);
  PixelData traverseTreePostOrder(VisualNode* node);

  void redrawAll();
  void drawHistogram(vector<float>& data, int color);

  void drawSolutionLine();

  /// Histograms
  void drawTimeHistogram();
  void drawDomainHistogram();
  void drawDomainReduction(unsigned l_vline, unsigned r_vline);
  void drawDepthAnalysisData();

  
  void drawVarData();

  /// Node Rate
  void drawNodeRate(unsigned leftmost_vline, unsigned rightmost_vline);

  /// select nodes that correspond to selected vline in pixel tree
  void selectNodesfromPT(unsigned vline);
  

public:
  PixelTreeCanvas(QWidget* parent, TreeCanvas& tc);
  ~PixelTreeCanvas(void);

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


/// ******* PIXEL_TREE_DIALOG ********

class PixelTreeDialog : public QDialog {
  Q_OBJECT

private:

  QAbstractScrollArea scrollArea;

  QCheckBox* time_cb;
  QCheckBox* domains_cb;
  QCheckBox* decision_vars_cb;
  QCheckBox* depth_analysis_cb;

  PixelTreeCanvas canvas;

Q_SIGNALS:

  void windowResized(void);

public:

  static const int MARGIN = 50;
  static const int DEPTH = 50;

  explicit PixelTreeDialog(TreeCanvas* tc);
  ~PixelTreeDialog(void);

protected:
  void resizeEvent(QResizeEvent * re);
};


/// ***********************************


#endif

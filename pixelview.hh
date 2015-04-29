#ifndef PIXEL_VIEW_HH
#define PIXEL_VIEW_HH

#include "treecanvas.hh"
#include <QImage>
#include <list>
#include <vector>


class PixelTreeCanvas;

/// ******* PIXEL_TREE_DIALOG ********

class PixelTreeDialog : public QDialog {
  Q_OBJECT

private:
  QVBoxLayout layout;
  QHBoxLayout controlLayout;
  
  QAbstractScrollArea scrollArea;

  QPushButton scaleUp;
  QPushButton scaleDown;

  QSpinBox compressionSB;
  
  PixelTreeCanvas* canvas;

public:

  static const int MARGIN = 50;
  static const int DEPTH = 50;
  

  PixelTreeDialog(TreeCanvas* tc);
  ~PixelTreeDialog(void);
};


/// ***********************************

class PixelData {
private:
  int   _idx;
  int   _depth;
  VisualNode* _node;
  bool  _selected;
public:
  PixelData(int idx, VisualNode* node, int depth)
  : _idx(idx), _node(node), _depth(depth) {};
  inline int idx() { return _idx; }
  inline int depth() { return _depth; }
  inline VisualNode* node() { return _node; }
};


/// ******** PIXEL_TREE_CANVAS ********


class PixelTreeCanvas : public QWidget {
  Q_OBJECT

private:
  TreeCanvas*     _tc;
  NodeAllocator*  _na;
  QImage*    _image;
  QLabel     qlabel;
  QPixmap         pixmap;

  QAbstractScrollArea*  _sa;
  QScrollBar*           _vScrollBar;

  /// Constants for a particular execution
  uint _nodeCount;

  /// Pixel Tree settings (changed through GUI)
    
  int _step = 2; // size of a 'pixel' in pixels
  int _step_y = 2; // size of a 'pixel' in pixels
  int approx_size = 1; // how many nodes per vertical line

  // int hist_height = 50;
  // int margin = 10;

  /// temp stuff for a Pixel Tree
  int   node_idx;
  int   x;
  float   group_time;
  float   group_domain; // average of domain size of nodes in a group
  float   group_domain_red;
  int   group_size;
  int   vline_idx; // same as x when _step = 1
  float alpha_factor;
  int   group_size_nonempty; // for calculating average

  int pt_height;
  int pt_width;

  /// Stuff specific for a particular pixel tree
  int vlines; /// width of pixel tree
  int max_depth;

  float* time_arr         = nullptr; // time for each vline
  float* domain_arr       = nullptr; // domain for each vline
  float* domain_red_arr   = nullptr; /// domain reduction for each vline

  std::vector<VisualNode*> nodes_selected;

  /// New Stuff
  std::vector<std::list<PixelData*>> pixelList;

public:

  static const int HIST_HEIGHT = 50;
  static const int MARGIN = 10;

private:

  /// Pixel Tree
  void constructTree(void);
  void drawPixelTree(void);
  void exploreNew(VisualNode* node, int depth);
  void freePixelList(std::vector<std::list<PixelData*>>& pixelList);

  void actuallyDraw(void);
  void drawHistogram(int idx, float* data, int l_vline, int r_vline, int color);

  /// Histograms
  void drawTimeHistogram(int l_vline, int r_vline);
  void drawDomainHistogram(int l_vline, int r_vline);
  void drawDomainReduction(int l_vline, int r_vline);

  /// Node Rate
  void drawNodeRate(int leftmost_vline, int rightmost_vline);

  void flush(void); /// make a final group

  /// auxiliary methods
  inline void drawPixel(int x, int y, int step, int color);

  /// select nodes that correspond to selected vline in pixel tree
  void selectNodesfromPT(int vline);
  

public:
  PixelTreeCanvas(QWidget* parent, TreeCanvas* tc);
  ~PixelTreeCanvas(void);

protected:
  void paintEvent(QPaintEvent* event);

  void mousePressEvent(QMouseEvent* me);

public Q_SLOTS:
  void scaleUp(void);
  void scaleDown(void);
  void compressionChanged(int value);
};


/// ***********************************


#endif

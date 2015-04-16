#ifndef PIXEL_VIEW_HH
#define PIXEL_VIEW_HH

#include "treecanvas.hh"
#include <QImage>


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
    
  int _step = 1; // size of a 'pixel' in pixels
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
  int*  intencity_arr = nullptr; // array of intencity for each pixel on vline
  int   group_size_nonempty; // for calculating average

  int pt_height;
  int pt_width;

  int call_stack_size = 0; // for debugging
  int max_stack_size = 0;// for debugging

  /// Stuff specific for a particular pixel tree
  int vlines; /// width of pixel tree
  int max_depth;

  float* time_arr         = nullptr; // time for each vline
  float* domain_arr       = nullptr; // domain for each vline
  float* domain_red_arr   = nullptr; /// domain reduction for each vline

  int node_selected;

public:

  static const int HIST_HEIGHT = 50;
  static const int MARGIN = 10;

private:

  /// Pixel Tree
  void drawPixelTree(void);
  void exploreNode(VisualNode* node, int depth);

  void drawHistogram(int idx, float* data, int color);

  /// Histograms
  void drawTimeHistogram(void);
  void drawDomainHistogram(void);
  void drawDomainReduction(void);

  /// Node Rate
  void drawNodeRate(void);

  void flush(void); /// make a final group

  /// auxiliary methods
  inline void drawPixel(int x, int y, int step, int color);

  /// select nodes that correspond to selected vline in pixel tree
  void selectNodesfromPT(int first, int last);
  

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

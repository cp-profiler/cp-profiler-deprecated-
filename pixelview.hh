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

  static const int MARGIN = 20;
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
  QImage*         _image;
  QLabel          qlabel;
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
  int   x;
  int   group_time;
  int   group_domain; // average of domain size of nodes in a group
  int   group_size;
  int   vline_idx; // same as x when _step = 1
  float alpha_factor;
  int*  intencity_arr; // array of intencity for each pixel on vline

  int pt_height;
  int pt_width;

  int call_stack_size = 0; // for debugging
  int max_stack_size = 0;// for debugging

  /// Stuff specific for a particular pixel tree
  int vlines; /// width of pixel tree
  int max_depth;

  int*   time_arr; // time for each vline
  float* domain_arr; // domain for each vline

  int    max_time; // max vline time
  float  max_domain;  

public:

  static const int HIST_HEIGHT = 50;
  static const int MARGIN = 10;

private:

  /// Pixel Tree
  void drawPixelTree(void);
  void exploreNode(VisualNode* node, int depth);

  /// Time Histogram
  void drawTimeHistogram(void);
  void drawDomainHistogram(void);

  /// auxiliary methods
  inline void drawPixel(int x, int y, int step, int color);
  

public:
  PixelTreeCanvas(QWidget* parent, TreeCanvas* tc);
  ~PixelTreeCanvas(void);

protected:
  void paintEvent(QPaintEvent* event);

public Q_SLOTS:
  void scaleUp(void);
  void scaleDown(void);
  void compressionChanged(int value);
  

};


/// ***********************************


#endif

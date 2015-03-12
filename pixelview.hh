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
  
  PixelTreeCanvas* canvas;

  


public:

  static const int MARGIN = 30;
  static const int DEPTH = 50;
  

  PixelTreeDialog(TreeCanvas* tc);
  ~PixelTreeDialog(void);
};


/// ***********************************


/// ******** PIXEL_TREE_CANVAS ********


class PixelTreeCanvas : public QWidget {
  Q_OBJECT

private:
  TreeCanvas* _tc;
  NodeAllocator* _na;
  QImage* _image;
  QLabel qlabel;
  QPixmap pixmap;

  QAbstractScrollArea* _sa;
  QScrollBar* _vScrollBar;

  int _step = 1;
  
  int approx_size = 1000; // approximation
  int group_size = 0;
  int group_depth = 0;


  uint _nodeCount;




  int call_stack_size = 0; // for debugging
  int max_stack_size = 0;// for debugging

  int x;

private:
  void draw(void);
  void exploreNode(VisualNode* node, int depth);
  

public:
  PixelTreeCanvas(QWidget* parent, TreeCanvas* tc);
  ~PixelTreeCanvas(void);

protected:
  void paintEvent(QPaintEvent* event);

public Q_SLOTS:
  void scaleUp(void);
  void scaleDown(void);
  

};


/// ***********************************


#endif

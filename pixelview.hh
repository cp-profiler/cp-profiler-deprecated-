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
  QLabel qlabel;
  QPixmap pixmap;
  QAbstractScrollArea scrollArea;

  NodeAllocator* _na;

  QImage* image;


  int MARGIN = 30;
  int DEPTH = 20;
  int STEP = 5;

  PixelTreeCanvas* canvas;

public:
  PixelTreeDialog(TreeCanvas* tc);
  // ~PixelTreeDialog(void);
protected:

  void draw(void);
  
};


/// ***********************************


/// ******** PIXEL_TREE_CANVAS ********


class PixelTreeCanvas : QWidget {

private:
  QImage* _image;

public:
  PixelTreeCanvas(QWidget* parent, QImage* image);

protected:
  void paintEvent(QPaintEvent* event);
  

};


/// ***********************************


#endif

#ifndef PIXEL_VIEW_HH
#define PIXEL_VIEW_HH

#include "treecanvas.hh"
#include <QImage>

class PixelTreeDialog : public QDialog {
  Q_OBJECT

private:
  QVBoxLayout layout;
  QLabel qlabel;
  QPixmap pixmap;
  QAbstractScrollArea scrollArea;

  NodeAllocator* _na;

  int STEP = 10;


  QImage* image;

public:
  PixelTreeDialog(TreeCanvas* tc);
  // ~PixelTreeDialog(void);
protected:
  void paintEvent(QPaintEvent* event);
  void draw(void);
};

#endif

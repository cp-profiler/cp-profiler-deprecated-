#include "pixelview.hh"

/// ******* PIXEL_TREE_DIALOG ********

PixelTreeDialog::PixelTreeDialog(TreeCanvas* tc)
  : QDialog(tc), _na(tc->na)
{

  uint numberOnNodes = tc->stats.solutions + tc->stats.failures
                       + tc->stats.choices + tc->stats.undetermined;

  int height = DEPTH * STEP;
  int width = numberOnNodes * STEP;

  this->resize(width + MARGIN, height + MARGIN);
  setLayout(&layout);
  layout.addWidget(&scrollArea);


  image = new QImage(width + MARGIN, height + MARGIN, QImage::Format_RGB888);

  canvas = new PixelTreeCanvas(&scrollArea, image);

  qlabel.show();


  draw();

  pixmap.fromImage(*image);
  qlabel.setPixmap(pixmap);

}

void
PixelTreeDialog::draw(void) {

  image->fill(qRgb(255, 255, 255));

  QStack<VisualNode*> stack;
  QStack<int> depth_stack;
  VisualNode* root = (*_na)[0];

  stack.push(root);
  depth_stack.push(1);

  int n = 1;
  int depth;

  while (stack.size() > 0) {
    
    qDebug() << "stack size: " << stack.size();

    if (stack.size() > 1000)
      break;

    VisualNode* node = stack.pop();
    depth = depth_stack.pop();

    // draw current

    for (uint i = 0; i < STEP; i++)
      for (uint j = 0; j < STEP; j++)
        image->setPixel(n + i, depth + j, qRgb(189, 149, 39));


    n += STEP;

    // if has children -> push all
    uint kids = node->getNumberOfChildren();
    for (uint i = 0; i < kids; ++i) {
      stack.push(node->getChild(*_na, i));
      depth_stack.push(depth + STEP);
    }
  }
  
}

/// ***********************************


/// ******** PIXEL_TREE_CANVAS ********

PixelTreeCanvas::PixelTreeCanvas(QWidget* parent, QImage* image)
  : QWidget(parent), _image(image)
{

  this->resize(image->width(), image->height());

}

void
PixelTreeCanvas::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.drawImage(0, 0, *_image);
}




/// ***********************************
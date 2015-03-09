#include "pixelview.hh"

PixelTreeDialog::PixelTreeDialog(TreeCanvas* tc)
  : QDialog(tc), _na(tc->na)
{
  this->resize(800, 600);
  setLayout(&layout);
//  layout.addWidget(&qlabel);


  image = new QImage(1000, 1000, QImage::Format_RGB888);

  qlabel.show();

  draw();


  pixmap.fromImage(*image);
  qlabel.setPixmap(pixmap);

}

void
PixelTreeDialog::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.drawImage(0, 0, *image);
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

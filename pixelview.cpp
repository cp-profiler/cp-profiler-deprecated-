#include "pixelview.hh"

/// ******* PIXEL_TREE_DIALOG ********

PixelTreeDialog::PixelTreeDialog(TreeCanvas* tc)
  : QDialog(tc)
{

  this->resize(600, 400);
  setLayout(&layout);
  layout.addWidget(&scrollArea);
  layout.addLayout(&controlLayout);

  controlLayout.addWidget(&scaleDown);
  controlLayout.addWidget(&scaleUp);

  canvas = new PixelTreeCanvas(&scrollArea, tc);

  connect(&scaleDown, SIGNAL(clicked()), canvas, SLOT(scaleDown()));
  connect(&scaleUp, SIGNAL(clicked()), canvas, SLOT(scaleUp()));

}

/// ***********************************


/// ******** PIXEL_TREE_CANVAS ********

PixelTreeCanvas::PixelTreeCanvas(QWidget* parent, TreeCanvas* tc)
  : QWidget(parent), _tc(tc), _na(tc->na)
{

  _sa = static_cast<QAbstractScrollArea*>(parentWidget());
  _vScrollBar = _sa->verticalScrollBar();
  _step;

  _nodeCount = tc->stats.solutions + tc->stats.failures
                       + tc->stats.choices + tc->stats.undetermined;

  int height = PixelTreeDialog::DEPTH * _step;
  int width = _nodeCount * _step;

  _image = new QImage(width + PixelTreeDialog::MARGIN,
                      height + PixelTreeDialog::MARGIN,
                      QImage::Format_RGB888);

  // connect(_vScrollBar, SIGNAL(valueChanged(int)), this, SLOT(draw(void)));
  // this->resize(_image->width(), _image->height());



  /// scrolling business
  _sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  _sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  _sa->setAutoFillBackground(true);
  
  draw();


  pixmap.fromImage(*_image);
  qlabel.setPixmap(pixmap);
  qlabel.show();

}

void
PixelTreeCanvas::paintEvent(QPaintEvent* event) {
  QPainter painter(this);

  _sa->horizontalScrollBar()->setRange(0, _image->width());
  _sa->verticalScrollBar()->setRange(0, _image->height());
  
  int xoff = _sa->horizontalScrollBar()->value();
  int yoff = _sa->verticalScrollBar()->value();

  painter.drawImage(0, 0, *_image, xoff, yoff);


}

void
PixelTreeCanvas::draw(void) {

  x = 0;
  delete _image;

  int height = PixelTreeDialog::DEPTH * _step;
  int width = _nodeCount * _step;

  
  

  _image = new QImage(width + PixelTreeDialog::MARGIN,
                      height + PixelTreeDialog::MARGIN,
                      QImage::Format_RGB888);

  this->resize(_image->width(), _image->height());

  _image->fill(qRgb(255, 255, 255));

  VisualNode* root = (*_na)[0];

  exploreNode(root, 1);

  qDebug() << "Max call stack size: " << max_stack_size;
  
}

void
PixelTreeCanvas::exploreNode(VisualNode* node, int depth) {
  call_stack_size++;

  if (max_stack_size < call_stack_size)
    max_stack_size = call_stack_size;

  // draw current
  for (uint i = 0; i < _step; i++)
    for (uint j = 0; j < _step; j++)
      _image->setPixel(x + i, depth + j, qRgb(189, 149, 39));

  x += _step;

  // for children
  uint kids = node->getNumberOfChildren();
  for (uint i = 0; i < kids; ++i) {
    exploreNode(node->getChild(*_na, i), depth + _step);
  }

  call_stack_size--;
}

void
PixelTreeCanvas::scaleUp(void) {
  _step++;
  draw();
}

void
PixelTreeCanvas::scaleDown(void) {
  if (_step <= 1) return;
  _step--;
  draw();
}




/// ***********************************

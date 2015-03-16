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

  scaleUp.setText("+");
  scaleDown.setText("-");

  QLabel* compLabel = new QLabel("compression");
  compLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  controlLayout.addWidget(compLabel);

  controlLayout.addWidget(&compressionSB);
  compressionSB.setMinimum(1);
  compressionSB.setMaximum(10000);

  canvas = new PixelTreeCanvas(&scrollArea, tc);

  connect(&scaleDown, SIGNAL(clicked()), canvas, SLOT(scaleDown()));
  connect(&scaleUp, SIGNAL(clicked()), canvas, SLOT(scaleUp()));

  QObject::connect(&compressionSB, SIGNAL(valueChanged(int)),
    canvas, SLOT(compressionChanged(int)));

  setAttribute(Qt::WA_QuitOnClose, true);
  setAttribute(Qt::WA_DeleteOnClose, true);

}

PixelTreeDialog::~PixelTreeDialog(void) {
  delete canvas;
}

PixelTreeCanvas::~PixelTreeCanvas(void) {
  delete _image;
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

  int height = tc->stats.maxDepth * _step;
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

  _sa->horizontalScrollBar()->setRange(0, _image->width()/approx_size - _sa->width());
  _sa->verticalScrollBar()->setRange(0, _image->height()/approx_size  - _sa->height());
  
  int xoff = _sa->horizontalScrollBar()->value();
  int yoff = _sa->verticalScrollBar()->value();

  // painter.eraseRect(event->rect());

  painter.drawImage(0, 0, *_image, xoff, yoff);
  

}

void
PixelTreeCanvas::draw(void) {

  x = 0;
  delete _image;

  int height = _tc->stats.maxDepth * _step;
  int width = _nodeCount * _step;

  _image = new QImage(width + PixelTreeDialog::MARGIN,
                      height + PixelTreeDialog::MARGIN,
                      QImage::Format_RGB888);

  _image->fill(qRgb(255, 255, 255));

  this->resize(_image->width(), _image->height());

  VisualNode* root = (*_na)[0];

  group_size = 0;

  exploreNode(root, 1);

  repaint();
  
}

/// *** Old implementation (averaging the depth)

// void
// PixelTreeCanvas::exploreNode(VisualNode* node, int depth) {
//   call_stack_size++;

//   if (max_stack_size < call_stack_size)
//     max_stack_size = call_stack_size;


//   // handle approximaiton
//   group_size++;
//   group_depth += depth;

//   if (group_size == approx_size) {


//     int y = group_depth / approx_size;
//     // draw current
//     for (uint i = 0; i < _step; i++)
//       for (uint j = 0; j < _step; j++)
//         _image->setPixel(x + i, y + j, qRgb(189, 149, 39));

//     x += _step;

//     group_size = 0;
//     group_depth = 0;
//   }

//   // for children
//   uint kids = node->getNumberOfChildren();
//   for (uint i = 0; i < kids; ++i) {
//     exploreNode(node->getChild(*_na, i), depth + _step);
//   }

//   call_stack_size--;
// }


void
PixelTreeCanvas::exploreNode(VisualNode* node, int depth) {
  call_stack_size++;

  if (max_stack_size < call_stack_size)
    max_stack_size = call_stack_size;

  Data* data = _tc->getData();
  DbEntry* entry = data->getEntry(node->getIndex(*_na));

  group_time += entry->node_time;

    // draw vertical line if a solution
  if (node->getStatus() == SOLVED) {
    for (uint j = 0; j < height(); j++)
      if (_image->pixel(x + 10, j) == qRgb(255, 255, 255))
        for (uint i = 0; i < _step; i++)
          _image->setPixel(x + i + 10, j, qRgb(0, 255, 0));
  }

  // draw current
  for (uint i = 0; i < _step; i++)
    for (uint j = 0; j < _step; j++)
      _image->setPixel(x + i + 10, depth + j, qRgba(255, 255, 255, group_time));

  // handle approximaiton
  group_size++;


  /// move to the right 
  if (group_size == approx_size) {
    x += _step;
    group_size = 0;
    group_time = 0;
  }

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

void
PixelTreeCanvas::compressionChanged(int value) {
  qDebug() << "compression is set to: " << value;
  approx_size = value;
  draw();
}




/// ***********************************

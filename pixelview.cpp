#include "pixelview.hh"

#include <cmath>

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

  if (time_arr != NULL) 
    delete time_arr;

  if (domain_arr != NULL)
    delete domain_arr;

  if (intencity_arr != NULL)
    delete intencity_arr;
}

/// ***********************************


/// ******** PIXEL_TREE_CANVAS ********

PixelTreeCanvas::PixelTreeCanvas(QWidget* parent, TreeCanvas* tc)
  : QWidget(parent), _tc(tc), _na(tc->na)
{

  _sa = static_cast<QAbstractScrollArea*>(parentWidget());
  _vScrollBar = _sa->verticalScrollBar();
  _step;

  time_arr      = NULL;
  domain_arr    = NULL;
  intencity_arr = NULL;

  _nodeCount = tc->stats.solutions + tc->stats.failures
                       + tc->stats.choices + tc->stats.undetermined;


  max_depth = tc->stats.maxDepth;
  int height = max_depth * _step;
  int width = _nodeCount * _step;

  // _image = new QImage(width + PixelTreeDialog::MARGIN,
  //                     height + PixelTreeDialog::MARGIN,
  //                     QImage::Format_RGB888);

  _image = NULL;

  // connect(_vScrollBar, SIGNAL(valueChanged(int)), this, SLOT(draw(void)));
  // this->resize(_image->width(), _image->height());



  /// scrolling business
  _sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  _sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  _sa->setAutoFillBackground(true);
  
  drawPixelTree();


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

  painter.drawImage(10, 10, *_image, xoff, yoff);
  

}

void
PixelTreeCanvas::drawPixelTree(void) {

  x = 0;
  delete _image;

  pt_height = max_depth * _step;
  pt_width = _nodeCount * _step;

  int img_width  = pt_width + 2 * MARGIN;
  int img_height = pt_height + 2 * (HIST_HEIGHT + _step) + 2 * MARGIN + 2 * MARGIN;

  _image = new QImage(img_width,
                      img_height,
                      QImage::Format_RGB888);

  _image->fill(qRgb(255, 255, 255));

  this->resize(_image->width(), _image->height());

  VisualNode* root = (*_na)[0];

  group_size   = 0;
  group_time   = 0;
  group_domain = 0;
  vline_idx    = 0;
  max_time     = 0;
  max_domain   = 0;
  
  vlines = ceil((float)_nodeCount / approx_size);

  if (time_arr != NULL) 
    delete time_arr;
  
  time_arr = new int[vlines];
  domain_arr = new float[vlines];
  intencity_arr = new int[max_depth];

  alpha_factor = 100.0 / approx_size;

  // set intencity_arr to zeros
  memset(intencity_arr, 0, max_depth * sizeof(int));

  exploreNode(root, 1);

  // flush(); TODO

  drawTimeHistogram();

  drawDomainHistogram();

  repaint();
  
}


void
PixelTreeCanvas::exploreNode(VisualNode* node, int depth) {
  call_stack_size++;

  if (max_stack_size < call_stack_size)
    max_stack_size = call_stack_size;

  Data* data = _tc->getData();
  DbEntry* entry = data->getEntry(node->getIndex(*_na));

  group_time   += entry->node_time;
  group_domain += entry->domain;

    // draw vertical line if a solution
  if (node->getStatus() == SOLVED) {
    for (uint j = 0; j < pt_height; j++)
      if (_image->pixel(x, j) == qRgb(255, 255, 255))
        for (uint i = 0; i < _step; i++)
          _image->setPixel(x + i, j, qRgb(0, 255, 0));
  }

  intencity_arr[depth - 1]++;

  // handle approximaiton
  group_size++;

  /// move to the right 
  if (group_size == approx_size) {

    /// record the time for each vline
    time_arr[vline_idx] = group_time;

    /// get average domain size for the group
    group_domain = group_domain / group_size;

    /// record domain size for each vline
    domain_arr[vline_idx] = group_domain;

    if (group_time > max_time) max_time = group_time;

    if (group_domain > max_domain) max_domain = group_domain;

    x += _step;
    vline_idx++;
    group_size = 0;
    group_time = 0;
    group_domain = 0;

    // int alpha; // temp variable for intencity level of each pixel

    // draw group
    for (uint d = 1; d <= max_depth; d++) {
      if (intencity_arr[d - 1] > 0) {
        int alpha = intencity_arr[d - 1] * alpha_factor;
        drawPixel(x, d * _step, _step, QColor::fromHsv(150, 100, 100 - alpha).rgba());
      }
        
    }
    
    // set intencity_arr to zeros
    memset(intencity_arr, 0, max_depth * sizeof(int));
  }

  // for children
  uint kids = node->getNumberOfChildren();
  for (uint i = 0; i < kids; ++i) {
    exploreNode(node->getChild(*_na, i), depth + 1);
  }

  call_stack_size--;
}

/// Draw time histogram underneath the pixel tree
void
PixelTreeCanvas::drawTimeHistogram(void) {

  float coeff = (float)HIST_HEIGHT / max_time;

  for (int i = 0; i < vlines; i++) {

    int val = time_arr[i] * coeff;

    /// horizontal line / 0 level
    for (int j = 0; j < _step; j++)
      _image->setPixel(i * _step + j,
                       pt_height + MARGIN + (HIST_HEIGHT + _step),
                       qRgb(150, 150, 150));

    // qDebug() << "timeValue: " << val;
    
    drawPixel(i * _step,
              pt_height + MARGIN + HIST_HEIGHT - val,
              _step,
              qRgb(150, 40, 150));
  }
}

void
PixelTreeCanvas::drawDomainHistogram(void) {

  float coeff = (float)HIST_HEIGHT / max_domain;

  for (int i = 0; i < vlines; i++) {
    int val = domain_arr[i] * coeff;

    for (int j = 0; j < _step; j++)
      _image->setPixel(i * _step + j,
                       pt_height + 2 * MARGIN + 2 * (HIST_HEIGHT + _step),
                       qRgb(150, 150, 150));

    qDebug() << "domainValue: " << val;

    drawPixel(i * _step,
              pt_height + 2 * MARGIN + (HIST_HEIGHT + _step) + HIST_HEIGHT - val,
              _step,
              qRgb(40, 150, 150));
  }

}

void
PixelTreeCanvas::scaleUp(void) {
  _step++;
  drawPixelTree();
}

void
PixelTreeCanvas::scaleDown(void) {
  if (_step <= 1) return;
  _step--;
  drawPixelTree();
}

void
PixelTreeCanvas::compressionChanged(int value) {
  qDebug() << "compression is set to: " << value;
  approx_size = value;
  drawPixelTree();
}

void
PixelTreeCanvas::drawPixel(int x, int y, int step, int color) {
  if (y < 0)
    return; /// TODO: fix later

  for (uint i = 0; i < _step; i++)
    for (uint j = 0; j < _step; j++)
      _image->setPixel(x + i, y + j, color);

  
}




/// ***********************************
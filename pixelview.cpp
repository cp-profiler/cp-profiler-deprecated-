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

  if (domain_red_arr != NULL)
    delete domain_red_arr;
}

/// ***********************************


/// ******** PIXEL_TREE_CANVAS ********

PixelTreeCanvas::PixelTreeCanvas(QWidget* parent, TreeCanvas* tc)
  : QWidget(parent), _tc(tc), _na(tc->na)
{

  _sa = static_cast<QAbstractScrollArea*>(parentWidget());
  _vScrollBar = _sa->verticalScrollBar();
  _step;

  time_arr        = NULL;
  domain_arr      = NULL;
  intencity_arr   = NULL;
  domain_red_arr  = NULL;

  _nodeCount = tc->stats.solutions + tc->stats.failures
                       + tc->stats.choices + tc->stats.undetermined;


  max_depth = tc->stats.maxDepth;
  int height = max_depth * _step;
  int width = _nodeCount * _step;

  _image = NULL;

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

  painter.drawImage(10, 10, *_image, xoff, yoff);
  

}

void
PixelTreeCanvas::drawPixelTree(void) {

  x = 0;
  delete _image;

  pt_height = max_depth * _step;
  pt_width = _nodeCount * _step;

  int img_width  = pt_width + 2 * MARGIN;
  // int img_height = pt_height + 4 * (HIST_HEIGHT + _step) + 3 * MARGIN + 3 * MARGIN;
  int img_height = MARGIN + 
                   pt_height + _step +
                   MARGIN +
                   HIST_HEIGHT + _step + /// Time Histogram
                   MARGIN +
                   HIST_HEIGHT + _step + /// Domain Histogram
                   MARGIN +
                   HIST_HEIGHT + _step + /// Domain Reduction Histogram
                   MARGIN +
                   HIST_HEIGHT + _step + /// Node Rate Histogram
                   MARGIN;

  qDebug() << "img_w: " << img_width << " img_h: " << img_height;

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
  group_domain_red    = 0;
  group_size_nonempty = 0;
  
  vlines = ceil((float)_nodeCount / approx_size);

  if (time_arr != NULL) 
    delete time_arr;
  
  time_arr = new float[vlines];
  domain_arr = new float[vlines];
  intencity_arr = new int[max_depth];
  domain_red_arr = new float[vlines];

  alpha_factor = 100.0 / approx_size;

  // set intencity_arr to zeros
  memset(intencity_arr, 0, max_depth * sizeof(int));

  exploreNode(root, 1);

  flush();

  // drawTimeHistogram();

  // drawDomainHistogram();

  // drawDomainReduction();

  drawNodeRate();

  repaint();
  
}

void
PixelTreeCanvas::flush(void) {

  qDebug() << "group size = " << group_size; 
  qDebug() << "nonempty size = " << group_size_nonempty; 

  if (group_size == 0)
    return;

  if (group_size_nonempty == 0) {
    group_domain      = -1;
    group_domain_red  = -1;
    group_time        = -1;
  } else {
    group_domain        = group_domain / group_size_nonempty;
    group_domain_red    = group_domain_red / group_size_nonempty;
  }

  domain_arr[vline_idx]     = group_domain;
  domain_red_arr[vline_idx] = group_domain_red;
  time_arr[vline_idx]       =  group_time;

}


void
PixelTreeCanvas::exploreNode(VisualNode* node, int depth) {
  DbEntry* entry;
  DbEntry* parent;

  call_stack_size++;

  if (max_stack_size < call_stack_size)
    max_stack_size = call_stack_size;

  Data* data = _tc->getData();

  entry  = data->getEntry(node->getIndex(*_na));

  if (!entry) {
    qDebug() << "entry do not exist\n";
  } else {

    if (entry->parent_sid == ~0u)
      parent = NULL;
    else
      parent = data->getEntry(node->getParent());

    group_time   += entry->node_time;
    group_domain += entry->domain;
    group_size_nonempty++;

    // if (group_size_nonempty == 2).
      

    if (parent) { 
      group_domain_red += parent->domain - entry->domain;
    }
      
      // draw vertical line if a solution
    if (node->getStatus() == SOLVED) {
      for (uint j = 0; j < pt_height; j++)
        if (_image->pixel(x, j) == qRgb(255, 255, 255))
          for (uint i = 0; i < _step; i++)
            _image->setPixel(x + i, j, qRgb(0, 255, 0));
    }


  }

  intencity_arr[depth - 1]++;

  // handle approximaiton
  group_size++;


  /// move to the right 
  if (group_size == approx_size) {


    /// get average domain size for the group
    if (group_size_nonempty == 0) {
      group_domain      = -1;
      group_domain_red  = -1;
      group_time        = -1;
    } else {
      group_domain        = group_domain / group_size_nonempty;
      group_domain_red    = group_domain_red / group_size_nonempty;
      
    }

    /// record stuff for each vline
    domain_arr[vline_idx]     = group_domain;
    domain_red_arr[vline_idx] = group_domain_red;
    time_arr[vline_idx]       = group_time;


    // draw group
    for (uint d = 1; d <= max_depth; d++) {
      if (intencity_arr[d - 1] > 0) {
        int alpha = intencity_arr[d - 1] * alpha_factor;
        drawPixel(x, d * _step, _step, QColor::fromHsv(150, 100, 100 - alpha).rgba());
      }
        
    }

    x += _step;
    vline_idx++;
    group_size = 0;
    group_time = 0;
    group_domain = 0;
    group_domain_red = 0;
    group_size_nonempty = 0;
    
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


  drawHistogram(0, time_arr, qRgb(150, 150, 40));
}

void
PixelTreeCanvas::drawDomainHistogram(void) {
  drawHistogram(1, domain_arr, qRgb(150, 40, 150));
}

void
PixelTreeCanvas::drawDomainReduction(void) {
  drawHistogram(2, domain_red_arr, qRgb(40, 150, 150));
}

void
PixelTreeCanvas::drawHistogram(int idx, float* data, int color) {


  /// coordinates for the top-left corner
  int x = 0;
  int y = (pt_height + _step) + MARGIN + idx * (HIST_HEIGHT + MARGIN + _step);

  /// work out maximum value
  int max_value = 0;

  for (int i = 0; i < vlines; i++) {
    if (data[i] > max_value) max_value = data[i];
  }

  float coeff = (float)HIST_HEIGHT / max_value;

  int zero_level = y + HIST_HEIGHT + _step;

  for (int i = 0; i < vlines; i++) {
    int val = data[i] * coeff;

    /// horizontal line for 0 level
    for (int j = 0; j < _step; j++)
      _image->setPixel(x + i * _step + j,
                       // pt_height + 2 * MARGIN + 2 * (HIST_HEIGHT + _step),
                       zero_level, 
                       qRgb(150, 150, 150));

    // qDebug() << "data[" << i << "]: " << data[i];

    // if (data[i] < 0) continue;

    drawPixel(x + i * _step,
              // pt_height + 2 * MARGIN + (HIST_HEIGHT + _step) + HIST_HEIGHT - val,
              y + HIST_HEIGHT - val,
              _step,
              color);

  }


}

void
PixelTreeCanvas::drawNodeRate(void) {
  Data* data = _tc->getData();
  std::vector<float>& node_rate = data->node_rate;
  std::vector<int>& nr_intervals = data->nr_intervals;

  int start_x = 0;
  int start_y = (pt_height + _step) + MARGIN + 3 * (HIST_HEIGHT + MARGIN + _step);

  float max_node_rate = *std::max_element(node_rate.begin(), node_rate.end());

  float coeff = (float)HIST_HEIGHT / max_node_rate;
  
  qDebug() << "max node rate: " << max_node_rate;

  int zero_level = start_y + HIST_HEIGHT + _step;

  for (int i = 0; i < vlines; i++) {
    for (int j = 0; j < _step; j++)
      _image->setPixel(start_x + i * _step + j,
                       // pt_height + 2 * MARGIN + 2 * (HIST_HEIGHT + _step),
                       zero_level, 
                       qRgb(150, 150, 150));
  }

  int x = 0;

  // /// loop through intervals
  // for (int i = 0; i < node_rate.size(); i++) {
  //   /// draw line for each interval
  //   for (int j = 0; j < node_rate[i]; j++) {
  //     drawPixel(start_x + x,
  //               // pt_height + 2 * MARGIN + (HIST_HEIGHT + _step) + HIST_HEIGHT - val,
  //               start_y + HIST_HEIGHT - value,
  //               _step,
  //               qRgb(40, 40, 150));
  //     x++;
  //   }
  // }

  for (int i = 1; i < nr_intervals.size(); i++) {
    float value = node_rate[i - 1] * coeff;
    for (int x = ceil((float)nr_intervals[i-1] / approx_size);
             x < ceil((float)nr_intervals[i]   / approx_size); x++) {
      drawPixel(start_x + x * _step,
                // pt_height + 2 * MARGIN + (HIST_HEIGHT + _step) + HIST_HEIGHT - val,
                start_y + HIST_HEIGHT - value,
                _step,
                qRgb(40, 40, 150));
      // qDebug() << "x: " << x;
    }
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
  // if (y < 0)
  //   return; /// TODO: fix later

  for (uint i = 0; i < _step; i++)
    for (uint j = 0; j < _step; j++)
      _image->setPixel(x + i, y + j, color);

  
}




/// ***********************************

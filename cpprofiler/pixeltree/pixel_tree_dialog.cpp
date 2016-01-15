#include "pixel_tree_dialog.hh"
#include "treecanvas.hh"
#include "pixel_tree_canvas.hh"

using namespace cpprofiler::pixeltree;

PixelTreeDialog::PixelTreeDialog(TreeCanvas* tc): QDialog(tc)
{

  canvas = new PixelTreeCanvas(&scrollArea, *tc);

  /// set Title
  this->setWindowTitle(QString::fromStdString(tc->getExecution()->getData()->getTitle()));

  QVBoxLayout* layout = new QVBoxLayout();
  QHBoxLayout* controlLayout = new QHBoxLayout();
  setLayout(layout);
  layout->addWidget(&scrollArea);
  layout->addLayout(controlLayout);


  /// ***** control panel *****
  QPushButton* scaleUp = new QPushButton(this);
  scaleUp->setText("+");

  QPushButton* scaleDown = new QPushButton(this);
  scaleDown->setText("-");

  QLabel* compLabel = new QLabel("compression");
  compLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QSpinBox* compressionSB = new QSpinBox(this);
  compressionSB->setRange(1, 10000);

  controlLayout->addWidget(scaleDown);
  controlLayout->addWidget(scaleUp);
  controlLayout->addWidget(compLabel);
  controlLayout->addWidget(compressionSB);

  QCheckBox* time_cb = new QCheckBox(this);
  time_cb->setText("time");
  time_cb->setCheckState(Qt::Checked);
  controlLayout->addWidget(time_cb);

  this->resize(INIT_WIDTH, INIT_HEIGHT);

  connect(time_cb, SIGNAL(stateChanged(int)), canvas, SLOT(toggleTimeHistogram(int)));

  QCheckBox* domains_cb = new QCheckBox(this);
  domains_cb->setText("domains");
  domains_cb->setCheckState(Qt::Checked);
  controlLayout->addWidget(domains_cb);
  connect(domains_cb, SIGNAL(stateChanged(int)), canvas, SLOT(toggleDomainsHistogram(int)));

  QCheckBox* depth_analysis_cb = new QCheckBox(this);
  depth_analysis_cb->setText("depth analysis");
  depth_analysis_cb->setCheckState(Qt::Checked);
  controlLayout->addWidget(depth_analysis_cb);
  connect(depth_analysis_cb, SIGNAL(stateChanged(int)), canvas, SLOT(toggleDepthAnalysisHistogram(int)));

  QCheckBox* decision_vars_cb = new QCheckBox(this);
  decision_vars_cb->setText("vars");
  decision_vars_cb->setCheckState(Qt::Checked);
  controlLayout->addWidget(decision_vars_cb);
  connect(decision_vars_cb, SIGNAL(stateChanged(int)), canvas, SLOT(toggleVarsHistogram(int)));

  connect(scaleDown, SIGNAL(clicked()), canvas, SLOT(scaleDown()));
  connect(scaleUp, SIGNAL(clicked()), canvas, SLOT(scaleUp()));
  connect(compressionSB, SIGNAL(valueChanged(int)),
    canvas, SLOT(compressionChanged(int)));

  connect(this, SIGNAL(windowResized()), canvas, SLOT(resizeCanvas()));

  setAttribute(Qt::WA_QuitOnClose, true);
  setAttribute(Qt::WA_DeleteOnClose, true);

}

void
PixelTreeDialog::resizeEvent(QResizeEvent*) {
  emit windowResized();
}
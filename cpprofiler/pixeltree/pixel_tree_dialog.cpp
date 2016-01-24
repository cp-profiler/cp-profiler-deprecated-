#include "pixel_tree_dialog.hh"
#include "treecanvas.hh"
#include "pixel_tree_canvas.hh"

using namespace cpprofiler::pixeltree;

PixelTreeDialog::PixelTreeDialog(TreeCanvas* tc): QDialog(tc)
{

  this->resize(INIT_WIDTH, INIT_HEIGHT);
  
  /// set Title
  this->setWindowTitle(QString::fromStdString(tc->getExecution()->getData()->getTitle()));

  auto layout = new QVBoxLayout();
  auto controlLayout = new QHBoxLayout();
  auto optionsLayout = new QHBoxLayout();

  setLayout(layout);
  layout->addWidget(&scrollArea);
  layout->addLayout(controlLayout);
  layout->addLayout(optionsLayout);

  canvas = new PixelTreeCanvas(&scrollArea, *tc);

  /// ***** control panel *****
  QPushButton* scaleUp = new QPushButton("+", this);
  QPushButton* scaleDown = new QPushButton("-", this);

  // TODO(maxim): make sure this gets deleted 
  QLabel* compLabel = new QLabel("compression");
  compLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QSpinBox* compressionSB = new QSpinBox(this);
  compressionSB->setRange(1, 10000);

  controlLayout->addWidget(scaleDown);
  controlLayout->addWidget(scaleUp);
  controlLayout->addWidget(compLabel);
  controlLayout->addWidget(compressionSB);

  QCheckBox* time_cb = new QCheckBox("time", this);
  time_cb->setCheckState(Qt::Checked);
  optionsLayout->addWidget(time_cb);

  connect(this, SIGNAL(windowResized()), canvas, SLOT(resizeCanvas()));

  connect(time_cb, SIGNAL(stateChanged(int)), canvas, SLOT(toggleTimeHistogram(int)));

  QCheckBox* domains_cb = new QCheckBox("domains", this);
  domains_cb->setCheckState(Qt::Checked);
  optionsLayout->addWidget(domains_cb);
  connect(domains_cb, SIGNAL(stateChanged(int)), canvas, SLOT(toggleDomainsHistogram(int)));

  QCheckBox* depth_analysis_cb = new QCheckBox("depth analysis", this);
  depth_analysis_cb->setCheckState(Qt::Checked);
  optionsLayout->addWidget(depth_analysis_cb);
  connect(depth_analysis_cb, SIGNAL(stateChanged(int)), canvas, SLOT(toggleDepthAnalysisHistogram(int)));

  QCheckBox* decision_vars_cb = new QCheckBox("vars", this);
  decision_vars_cb->setCheckState(Qt::Checked);
  optionsLayout->addWidget(decision_vars_cb);
  connect(decision_vars_cb, SIGNAL(stateChanged(int)), canvas, SLOT(toggleVarsHistogram(int)));

  QCheckBox* bj_analysis_cb = new QCheckBox("backjumps", this);
  bj_analysis_cb->setCheckState(Qt::Checked);
  optionsLayout->addWidget(bj_analysis_cb);
  connect(bj_analysis_cb, SIGNAL(stateChanged(int)), canvas, SLOT(toggleBjHistogram(int)));


  connect(scaleDown, SIGNAL(clicked()), canvas, SLOT(scaleDown()));
  connect(scaleUp, SIGNAL(clicked()), canvas, SLOT(scaleUp()));
  connect(compressionSB, SIGNAL(valueChanged(int)),
    canvas, SLOT(compressionChanged(int)));

  setAttribute(Qt::WA_QuitOnClose, true);
  setAttribute(Qt::WA_DeleteOnClose, true);

}

void
PixelTreeDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  emit windowResized();
}
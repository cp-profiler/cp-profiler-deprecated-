
/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "pixel_tree_dialog.hh"
#include "treecanvas.hh"
#include "pixel_tree_canvas.hh"
#include "data.hh"

using namespace cpprofiler::pixeltree;

PixelTreeDialog::PixelTreeDialog(TreeCanvas* tc) : QDialog(tc) {
  this->resize(INIT_WIDTH, INIT_HEIGHT);

  this->setWindowTitle(
      QString::fromStdString(tc->getExecution()->getData()->getTitle()));

  canvas_ = new PixelTreeCanvas(&scrollArea, *tc, m_infoPanel);

  auto layout = new QVBoxLayout();
  setLayout(layout);

  layout->addWidget(&scrollArea);

  /// ********************
  /// ******* INFO *******
  /// ********************
  {
    auto infoLayout = new QHBoxLayout();
    layout->addLayout(infoLayout);

    auto info_label = new QLabel("info: ");
    info_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    infoLayout->addWidget(info_label);
    infoLayout->addWidget(m_infoPanel.info_label());
  }
  /// ********************
  /// ***** CONTROLS *****
  /// ********************
  {
    auto controlLayout = new QHBoxLayout();
    layout->addLayout(controlLayout);

    auto scaleDown = new QPushButton("-", this);
    controlLayout->addWidget(scaleDown);
    connect(scaleDown, &QPushButton::clicked, canvas_,
            &PixelTreeCanvas::scaleDown);

    auto scaleUp = new QPushButton("+", this);
    controlLayout->addWidget(scaleUp);
    connect(scaleUp, &QPushButton::clicked, canvas_, &PixelTreeCanvas::scaleUp);

    auto compLabel = new QLabel("compression");
    compLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    controlLayout->addWidget(compLabel);

    auto compressionSB = new QSpinBox(this);
    compressionSB->setRange(1, 10000);
    controlLayout->addWidget(compressionSB);
    connect(compressionSB, SIGNAL(valueChanged(int)), canvas_,
          SLOT(compressionChanged(int)));

    auto dfs_type_cb = new QComboBox();
    dfs_type_cb->addItem("pre-order");
    dfs_type_cb->addItem("post-order");
    controlLayout->addWidget(dfs_type_cb);
    connect(dfs_type_cb, SIGNAL(currentTextChanged(const QString&)), canvas_,
          SLOT(changeTraversalType(const QString&)));
  }

  /// *******************
  /// ***** OPTIONS *****
  /// *******************
  {
    auto optionsLayout = new QHBoxLayout();
    layout->addLayout(optionsLayout);

    auto time_cb = new QCheckBox("time", this);
    time_cb->setCheckState(Qt::Checked);
    optionsLayout->addWidget(time_cb);

    connect(time_cb, SIGNAL(stateChanged(int)), canvas_,
            SLOT(toggleTimeHistogram(int)));

    auto domains_cb = new QCheckBox("domains", this);
    domains_cb->setCheckState(Qt::Unchecked);
    optionsLayout->addWidget(domains_cb);
    connect(domains_cb, SIGNAL(stateChanged(int)), canvas_,
            SLOT(toggleDomainsHistogram(int)));

    auto depth_analysis_cb = new QCheckBox("depth analysis", this);
    depth_analysis_cb->setCheckState(Qt::Checked);
    optionsLayout->addWidget(depth_analysis_cb);
    connect(depth_analysis_cb, SIGNAL(stateChanged(int)), canvas_,
            SLOT(toggleDepthAnalysisHistogram(int)));

    auto decision_vars_cb = new QCheckBox("vars", this);
    decision_vars_cb->setCheckState(Qt::Checked);
    optionsLayout->addWidget(decision_vars_cb);
    connect(decision_vars_cb, SIGNAL(stateChanged(int)), canvas_,
            SLOT(toggleVarsHistogram(int)));

    auto bj_analysis_cb = new QCheckBox("backjumps", this);
    bj_analysis_cb->setCheckState(Qt::Checked);
    optionsLayout->addWidget(bj_analysis_cb);
    connect(bj_analysis_cb, SIGNAL(stateChanged(int)), canvas_,
            SLOT(toggleBjHistogram(int)));
  }

  connect(this, SIGNAL(signalPixelSelected(int)), canvas_,
          SLOT(setPixelSelected(int)));

  connect(this, SIGNAL(windowResized()), canvas_, SLOT(resizeCanvas()));

  setAttribute(Qt::WA_QuitOnClose, true);
  setAttribute(Qt::WA_DeleteOnClose, true);
}

void PixelTreeDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  emit windowResized();
}

void PixelTreeDialog::setPixelSelected(int gid) {
  emit signalPixelSelected(gid);
}

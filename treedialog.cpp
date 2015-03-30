#include "treedialog.hh"
#include "nodewidget.hh"

TreeDialog::TreeDialog(ReceiverThread* receiver, const TreeCanvas::CanvasType type, Gist* gist) : QDialog(gist),
  prt_gist(gist)
{

  layout = new QGridLayout(this);
  nc_layout = new QVBoxLayout();
  status_layout = new QVBoxLayout();

  main_layout = new QHBoxLayout();

  setLayout(main_layout);
  main_layout->addLayout(layout);
  main_layout->addLayout(status_layout);
  
  scrollArea = new QAbstractScrollArea(this);

  _tc = new TreeCanvas(layout, receiver, type, scrollArea->viewport());

  layout->addWidget(scrollArea, 0, 0, 1, 1);
  layout->addWidget(_tc->scaleBar, 0, 1, Qt::AlignHCenter);

  scrollArea->viewport()->setLayout(nc_layout);

  nc_layout->addWidget(_tc);

  buildMenu();


  /// *********** Status Bar ************

  statusBar = new QStatusBar(this);

  QWidget* stw = new QWidget();
  statusBar->addPermanentWidget(stw);
  layout->addWidget(statusBar);

  QHBoxLayout* hbl = new QHBoxLayout();
  hbl->setContentsMargins(0,0,0,0);

  stw->setLayout(hbl);

  hbl->addWidget(new QLabel("Depth:"));
  depthLabel = new QLabel("0");
  hbl->addWidget(depthLabel);

  hbl->addWidget(new NodeWidget(SOLVED));
  solvedLabel = new QLabel("0");
  hbl->addWidget(solvedLabel);

  hbl->addWidget(new NodeWidget(FAILED));
  failedLabel = new QLabel("0");
  hbl->addWidget(failedLabel);

  hbl->addWidget(new NodeWidget(BRANCH));
  choicesLabel = new QLabel("0");
  hbl->addWidget(choicesLabel);

  hbl->addWidget(new NodeWidget(UNDETERMINED));
  openLabel = new QLabel("0");
  hbl->addWidget(openLabel);

  
  statusBar->showMessage("Ready");


  /// ***********************************

  connectSignals();

  resize(500, 400);
  show();
}

TreeDialog::~TreeDialog() {
  delete _tc;
}

void
TreeDialog::buildMenu(void) {

  menuBar = new QMenuBar(0);

    // Don't add the menu bar on Mac OS X
  #ifndef Q_WS_MAC
    layout->setMenuBar(menuBar);
  #endif

  QMenu* nodeMenu = menuBar->addMenu(tr("&Node"));
  nodeMenu->addAction(prt_gist->labelBranches);
  nodeMenu->addAction(prt_gist->navUp);
  nodeMenu->addAction(prt_gist->navDown);
  nodeMenu->addAction(prt_gist->navLeft);
  nodeMenu->addAction(prt_gist->navRight);
  nodeMenu->addAction(prt_gist->navRoot);

}

void
TreeDialog::connectSignals(void) {

  connect(scrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            _tc, SLOT(scroll(void)));
  connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
            _tc, SLOT(scroll(void)));

}

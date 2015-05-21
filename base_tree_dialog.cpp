#include "base_tree_dialog.hh"
#include "nodewidget.hh"
#include "treecanvas.hh"

BaseTreeDialog::BaseTreeDialog(ReceiverThread* receiver, const CanvasType type, Gist* gist) :
  QDialog(gist), ptr_gist(gist), menuBar{nullptr}, nodeMenu{nullptr} {

  layout = new QGridLayout(this);
  nc_layout = new QVBoxLayout();
  status_layout = new QVBoxLayout();

  main_layout = new QHBoxLayout();

  // setLayout(main_layout);          /// TODO: find out which layout is already set
  // main_layout->addLayout(layout);  /// TODO: find out who is a `parent` of layout
  main_layout->addLayout(status_layout);
  
  scrollArea = new QAbstractScrollArea(this);


  // data is created here
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

  hbl = new QHBoxLayout();
  hbl->setContentsMargins(0,0,0,0);

  stw->setLayout(hbl);
  
  statusBar->showMessage("Ready");

  /// ***********************************

  connectSignals();

  resize(500, 400);
  show();

}

BaseTreeDialog::~BaseTreeDialog() {
  delete _tc;
}

void
BaseTreeDialog::buildMenu(void) {

  menuBar = new QMenuBar(this);

    // Don't add the menu bar on Mac OS X
  #ifndef Q_WS_MAC
    layout->setMenuBar(menuBar);
  #endif

  nodeMenu = menuBar->addMenu(tr("&Node"));

  nodeMenu->addAction(ptr_gist->labelBranches);
  nodeMenu->addAction(ptr_gist->labelPath);
  nodeMenu->addAction(ptr_gist->navUp);
  nodeMenu->addAction(ptr_gist->navDown);
  nodeMenu->addAction(ptr_gist->navLeft);
  nodeMenu->addAction(ptr_gist->navRight);
  nodeMenu->addAction(ptr_gist->navRoot);

  nodeMenu->addAction(ptr_gist->hideFailed);
  nodeMenu->addAction(ptr_gist->unhideAll);

}

void
BaseTreeDialog::connectSignals(void) {

  connect(_tc, SIGNAL(statusChanged(VisualNode*, const Statistics&, bool)),
          this, SLOT(statusChanged(VisualNode*, const Statistics&, bool)));

  connect(scrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            _tc, SLOT(scroll(void)));
  connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
            _tc, SLOT(scroll(void)));

}

void
BaseTreeDialog::statusChangedShared(bool finished) {
  if (finished) {
    /// add total time to 'Done' label
    QString t;
    unsigned long long totalTime = _tc->getData()->getTotalTime();

    const int MILLION = 1000000;
    float seconds = (float)totalTime / MILLION; /// microseconds to seconds

    t.setNum(seconds);
    statusBar->showMessage("Done in " + t + "s");

    qDebug() << "Done in " + t + "s";

    /// no need to change stats after done
    disconnect(_tc, SIGNAL(statusChanged(VisualNode*, const Statistics&, bool)),
          this, SLOT(statusChanged(VisualNode*, const Statistics&, bool)));

  } else {
    statusBar->showMessage("Searching");
  }
}

void
BaseTreeDialog::setTitle(const std::string& file_name) {
  QString title(file_name.c_str());
  this->setWindowTitle(title);
}
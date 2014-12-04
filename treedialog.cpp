#include "treedialog.hh"

TreeDialog::TreeDialog(RecieverThread* reciever, Gist* gist) : QDialog(gist),
  prt_gist(gist)
{

  layout = new QGridLayout(this);
  nc_layout = new QVBoxLayout();

  setLayout(layout);
  
  scrollArea = new QAbstractScrollArea(this);

  _tc = new TreeCanvas(layout, reciever, scrollArea->viewport());

  layout->addWidget(scrollArea, 0, 0, -1, 1);
  layout->addWidget(_tc->scaleBar, 1, 1, Qt::AlignHCenter);

  scrollArea->viewport()->setLayout(nc_layout);

  nc_layout->addWidget(_tc);

  buildMenu();

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
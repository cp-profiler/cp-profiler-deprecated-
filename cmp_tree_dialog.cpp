#include "cmp_tree_dialog.hh"
#include "nodewidget.hh"
#include "treecomparison.hh"

CmpTreeDialog::CmpTreeDialog(ReceiverThread* receiver, const CanvasType type, Gist* gist,
                             TreeCanvas* tc1, TreeCanvas* tc2)
: BaseTreeDialog(receiver, type, gist) {


  hbl->addWidget(new NodeWidget(MERGING));
  mergedLabel = new QLabel("0");
  hbl->addWidget(mergedLabel);

  _comparison = new TreeComparison();

  nodeMenu->addAction(ptr_gist->navFirstPentagon);
  nodeMenu->addAction(ptr_gist->navNextPentagon);
  nodeMenu->addAction(ptr_gist->navPrevPentagon);
  connect(ptr_gist->navFirstPentagon, SIGNAL(triggered()), this, SLOT(navFirstPentagon()));
  connect(ptr_gist->navNextPentagon, SIGNAL(triggered()), this, SLOT(navNextPentagon()));
  connect(ptr_gist->navPrevPentagon, SIGNAL(triggered()), this, SLOT(navPrevPentagon()));

  _comparison->compare(tc1, tc2, _tc);

  mergedLabel->setNum(_comparison->get_no_pentagons());
  statusChangedShared(true);

}

CmpTreeDialog::~CmpTreeDialog(void) {
  delete _comparison;
}

void
CmpTreeDialog::statusChanged(VisualNode*, const Statistics& stats, bool finished) {

  statusChangedShared(finished);

}

void
CmpTreeDialog::navFirstPentagon(void) {
  const std::vector<VisualNode*>& pentagons = _comparison->pentagons();

  if (pentagons.size() == 0) {
    qDebug() << "warning: pentagons.size() == 0";
    return;
  }

  _tc->setCurrentNode(pentagons[0]);
  _tc->centerCurrentNode();

}

void
CmpTreeDialog::navNextPentagon(void) {

  _tc->navNextPentagon();

}

void
CmpTreeDialog::navPrevPentagon(void) {

  _tc->navNextPentagon(true);
}
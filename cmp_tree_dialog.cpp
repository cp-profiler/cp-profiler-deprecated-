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



  _comparison->compare(tc1, tc2, _tc);

  mergedLabel->setNum(_comparison->get_no_pentagons());

}

CmpTreeDialog::~CmpTreeDialog(void) {
  delete _comparison;
}

void
CmpTreeDialog::statusChanged(VisualNode*, const Statistics& stats, bool finished) {

  statusChangedShared(finished);

}
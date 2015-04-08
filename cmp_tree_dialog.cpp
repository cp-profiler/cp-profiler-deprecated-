#include "cmp_tree_dialog.hh"
#include "nodewidget.hh"
#include "treecomparison.hh"

CmpTreeDialog::CmpTreeDialog(ReceiverThread* receiver, const TreeCanvas::CanvasType type, Gist* gist,
                             TreeCanvas* tc1, TreeCanvas* tc2)
: TreeDialog(receiver, type, gist) {


  hbl->addWidget(new NodeWidget(MERGING));
  mergedLabel = new QLabel("0");
  hbl->addWidget(mergedLabel);

  _comparison = new TreeComparison();

  _comparison->compare(tc1, tc2, _tc);

}

CmpTreeDialog::~CmpTreeDialog(void) {
  delete _comparison;
}
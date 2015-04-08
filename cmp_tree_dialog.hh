#ifndef CMP_TREE_DIALOG_HH
#define CMP_TREE_DIALOG_HH


#include "base_tree_dialog.hh"


class Gist;
class TreeComparison;

class CmpTreeDialog : public BaseTreeDialog {

private:

  TreeComparison* _comparison;

public:

  CmpTreeDialog(ReceiverThread* receiver, const TreeCanvas::CanvasType type, Gist* gist,
                TreeCanvas *tc1, TreeCanvas *tc2);
  ~CmpTreeDialog();

};

#endif
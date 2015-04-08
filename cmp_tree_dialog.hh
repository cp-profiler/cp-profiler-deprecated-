#ifndef CMP_TREE_DIALOG_HH
#define CMP_TREE_DIALOG_HH


#include "treedialog.hh"


class Gist;
class TreeComparison;

class CmpTreeDialog : public TreeDialog {

private:

  TreeComparison* _comparison;

public:

  CmpTreeDialog(ReceiverThread* receiver, const TreeCanvas::CanvasType type, Gist* gist,
                TreeCanvas *tc1, TreeCanvas *tc2);
  ~CmpTreeDialog();

};

#endif
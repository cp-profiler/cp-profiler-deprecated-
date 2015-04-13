#ifndef CMP_TREE_DIALOG_HH
#define CMP_TREE_DIALOG_HH

#include "base_tree_dialog.hh"

class Gist;
class TreeComparison;

class CmpTreeDialog : public BaseTreeDialog {
Q_OBJECT

private:

  TreeComparison* _comparison;

public:

  CmpTreeDialog(ReceiverThread* receiver, const CanvasType type, Gist* gist,
                TreeCanvas *tc1, TreeCanvas *tc2);
  ~CmpTreeDialog();

private Q_SLOTS:
  void statusChanged(VisualNode*, const Statistics& stats, bool finished);

  /// Move selection to first pentagon (for a Merged tree)
  void navFirstPentagon(void);
  void navNextPentagon(void);
  void navPrevPentagon(void);

};

#endif
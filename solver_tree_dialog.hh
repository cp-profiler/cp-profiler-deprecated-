#ifndef SOLVER_TREE_DIALOG_HH
#define SOLVER_TREE_DIALOG_HH

#include <QStatusBar>
#include "base_tree_dialog.hh"

class Gist;
class ReceiverThread;

class SolverTreeDialog : public BaseTreeDialog {
Q_OBJECT

private:

  /// Status bar label for number of solutions
  QLabel* depthLabel;
  QLabel* solvedLabel;
  QLabel* failedLabel;
  QLabel* choicesLabel;
  QLabel* openLabel;

public:

  SolverTreeDialog(ReceiverThread* receiver, const TreeCanvas::CanvasType type, Gist* gist);

  ~SolverTreeDialog();


private Q_SLOTS:
  void statusChanged(VisualNode*, const Statistics& stats, bool finished);

};


#endif
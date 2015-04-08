#ifndef SOLVER_TREE_DIALOG_HH
#define SOLVER_TREE_DIALOG_HH

#include <QStatusBar>
#include "base_tree_dialog.hh"

class Gist;
class ReceiverThread;

class SolverTreeDialog : public BaseTreeDialog {
Q_OBJECT
  
public:

  SolverTreeDialog(ReceiverThread* receiver, const TreeCanvas::CanvasType type, Gist* gist);

  ~SolverTreeDialog();

};


#endif

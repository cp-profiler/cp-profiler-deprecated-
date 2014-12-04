#ifndef TREE_DIALOG_HH
#define TREE_DIALOG_HH

#include "treecanvas.hh"
#include "recieverthread.hh"

class Gist;

class TreeDialog : public QDialog {
// Q_OBJECT
private:

  QGridLayout* layout;
  QVBoxLayout* nc_layout;
  QAbstractScrollArea* scrollArea;

  Gist* prt_gist;

  TreeCanvas* _tc;

  /// A menu bar
  QMenuBar* menuBar;

  void buildMenu(void);
  void connectSignals(void);

public:

  TreeDialog(RecieverThread* reciever, Gist* gist);

  ~TreeDialog();


  /// **** GETTERS ****

  TreeCanvas* getCanvas(void) { return _tc; }

};


#endif

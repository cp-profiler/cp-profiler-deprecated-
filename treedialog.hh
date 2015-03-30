#ifndef TREE_DIALOG_HH
#define TREE_DIALOG_HH

#include <QStatusBar>

#include "treecanvas.hh"
#include "receiverthread.hh"

class Gist;

class TreeDialog : public QDialog {
// Q_OBJECT
private:

  QHBoxLayout* main_layout;
  QVBoxLayout* status_layout;
  QGridLayout* layout;
  QVBoxLayout* nc_layout;
  QAbstractScrollArea* scrollArea;

  Gist* prt_gist;

  TreeCanvas* _tc;

  /// A menu bar
  QMenuBar* menuBar;


  /// Status Bar
  // QStatusBar* statusBar;
  QStatusBar* statusBar;

  /// Status bar label for number of solutions
  QLabel* depthLabel;
  QLabel* solvedLabel;
  QLabel* failedLabel;
  QLabel* choicesLabel;
  QLabel* openLabel;


  void buildMenu(void);
  void connectSignals(void);

public:

  TreeDialog(ReceiverThread* receiver, const TreeCanvas::CanvasType type, Gist* gist);

  ~TreeDialog();


  /// **** GETTERS ****

  TreeCanvas* getCanvas(void) { return _tc; }

};


#endif

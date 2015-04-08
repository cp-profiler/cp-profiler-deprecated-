#ifndef TREE_DIALOG_HH
#define TREE_DIALOG_HH

#include <QStatusBar>

#include "treecanvas.hh"


class Gist;
class ReceiverThread;

class TreeDialog : public QDialog {
Q_OBJECT
private:

  QHBoxLayout* main_layout;
  QVBoxLayout* status_layout;
  QGridLayout* layout;
  QVBoxLayout* nc_layout;
  QAbstractScrollArea* scrollArea;

  Gist* prt_gist;

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

protected:

  TreeCanvas* _tc;

  QLabel* mergedLabel;
  QHBoxLayout* hbl;

public:

  TreeDialog(ReceiverThread* receiver, const TreeCanvas::CanvasType type, Gist* gist);

  ~TreeDialog();


  /// **** GETTERS ****

  void setTitle(const char* file_name);
  TreeCanvas* getCanvas(void) { return _tc; }

  private Q_SLOTS:
  /// The status has changed (e.g., new solutions have been found)
  void statusChanged(VisualNode*, const Statistics& stats, bool finished);
};


#endif

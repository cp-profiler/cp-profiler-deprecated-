#ifndef CMP_TREE_DIALOG_HH
#define CMP_TREE_DIALOG_HH

#include <QTableWidget>
#include "base_tree_dialog.hh"

class Gist;
class TreeComparison;

class PentListWindow : public QDialog {
Q_OBJECT

friend class CmpTreeDialog;

private:

  QTableWidget _histTable;
  // const std::vector<VisualNode*>* p_pentagons;

private:
  void createList(const std::vector<VisualNode*>& pentagons,
                  const std::vector<std::pair<unsigned int, unsigned int>>& pentSize);


public:
  PentListWindow(QWidget* parent);
};

class CmpTreeDialog : public BaseTreeDialog {
Q_OBJECT

private:

  TreeComparison* _comparison;

  QMenu* analysisMenu;

  PentListWindow pentListWindow;


  /// ******* Actions *******
  /// Navigate to first pentagon
  QAction* _navFirstPentagon;
  /// Navigate to next pentagon
  QAction* _navNextPentagon;
  /// Navigate to prev pentagon
  QAction* _navPrevPentagon;
  /// Show pentagon histogram
  QAction* _showPentagonHist;

  /// ***********************

private:

  void addActions(void);

public:

  CmpTreeDialog(ReceiverThread* receiver, const CanvasType& type, Gist* gist,
                TreeCanvas *tc1, TreeCanvas *tc2);
  ~CmpTreeDialog();

private Q_SLOTS:
  void statusChanged(VisualNode*, const Statistics& stats, bool finished);

  /// Pentagon navigation
  void navFirstPentagon(void);
  void navNextPentagon(void);
  void navPrevPentagon(void);

  void showPentagonHist(void);
  void selectPentagon(int row, int);

};



#endif
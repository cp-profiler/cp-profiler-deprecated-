/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef CMP_TREE_DIALOG_HH
#define CMP_TREE_DIALOG_HH

#include <QTableWidget>
#include "base_tree_dialog.hh"
#include "execution.hh"

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
  explicit PentListWindow(QWidget* parent);
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

    CmpTreeDialog(Execution* execution, const CanvasType& type, //Gist* gist,
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

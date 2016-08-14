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
#include <QDialog>
#include <QLayout>

struct PentagonItem;
class Gist;
class TreeComparison;
class CmpTreeDialog;
class Execution;
class QStatusBar;
class QMenuBar;
class TreeCanvas;
class QLabel;
class VisualNode;
class Statistics;

class PentListWindow : public QDialog {
Q_OBJECT

friend class CmpTreeDialog;

private:

  QTableWidget _pentagonTable;
  QTableWidget _nogoodTable;

  const std::vector<PentagonItem>& _items;
  const TreeComparison& comparison_;

private:
  void createList(); /// make this a free function
  void populateNogoodTable(const std::vector<int>& nogoods);


public:
  PentListWindow(CmpTreeDialog* parent, const std::vector<PentagonItem>& items);
};

class CmpTreeDialog : public QDialog {
Q_OBJECT

private:

  QGridLayout* layout;

  QStatusBar* statusBar;

  TreeComparison* comparison_;

  TreeCanvas* m_Canvas;

private:

  void addActions(QMenu* nodeMenu, QMenu* analysisMenu);

  void setTitle(QString title);

public:

  CmpTreeDialog(QWidget* parent, Execution* execution, bool withLabels,
                const Execution& ex1, const Execution& ex2);

  ~CmpTreeDialog();

  void saveComparisonStatsTo(const QString& file_name);
  void selectPentagon(int row);

  const TreeComparison& comparison() { return *comparison_; }

private Q_SLOTS:
  /// The status has changed (e.g., new solutions have been found)
  void statusChanged(VisualNode*, const Statistics& stats, bool finished);

  /// Pentagon navigation
  void navFirstPentagon();
  void navNextPentagon();
  void navPrevPentagon();

  void showPentagonHist();
  void saveComparisonStats();

  void showResponsibleNogoods();

};



#endif

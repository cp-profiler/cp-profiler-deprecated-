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
#include <QMainWindow>
#include <QDialog>
#include <memory>

#include "nogoodtable.hh"

struct PentagonItem;
class Gist;
class ComparisonResult;
class CmpTreeDialog;
class Execution;
class QStatusBar;
class QMenuBar;
class TreeCanvas;
class QLabel;
class VisualNode;
class Statistics;
class QGridLayout;

class PentListWindow : public QDialog {
Q_OBJECT

private:

  QTableWidget _pentagonTable;
  NogoodTableView* _nogoodTable;

  const ComparisonResult& cmp_result;
  const std::vector<PentagonItem>& _items;

private:
  
  void populateNogoodTable(QStandardItemModel* model, const std::vector<NodeUID>& nogoods);

public:
  PentListWindow(CmpTreeDialog* parent, const ComparisonResult& items);
  void createList(); /// TODO(maxim): make this a free function?
};

class CmpTreeDialog : public QMainWindow {
Q_OBJECT

private:

  std::unique_ptr<ComparisonResult> m_Cmp_result;

  std::unique_ptr<TreeCanvas> m_Canvas;

  QGridLayout* layout;

  // QStatusBar* statusBar;

  bool expand_expressions;


private:

  void addActions(QMenu* nodeMenu, QMenu* analysisMenu);

  void setTitle(QString title);

public:

  CmpTreeDialog(QWidget* parent, Execution* execution, bool withLabels,
                const Execution& ex1, const Execution& ex2);

  ~CmpTreeDialog();

  void saveComparisonStatsTo(const QString& file_name);
  void selectPentagon(int row);
  TreeCanvas* getCanvas();

public Q_SLOTS:

  void showResponsibleNogoods();

private Q_SLOTS:

  /// Pentagon navigation
  void navFirstPentagon();
  void navNextPentagon();
  void navPrevPentagon();

  void showPentagonHist();
  void saveComparisonStats();

};



#endif

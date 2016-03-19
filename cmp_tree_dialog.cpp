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

#include "cmp_tree_dialog.hh"
#include "nodewidget.hh"

#include <utility> // pair
#include <algorithm>
#include <cmath>

/// TODO(maxim): use normal Gist window for comparison instead???

CmpTreeDialog::CmpTreeDialog(QWidget* parent, Execution* execution, bool withLabels,
                             TreeCanvas* tc1, TreeCanvas* tc2)
    : BaseTreeDialog(parent, execution, CanvasType::MERGED),
comparison_{withLabels}, analysisMenu{nullptr}, pentListWindow{this} {

  hbl->addWidget(new NodeWidget(MERGING));
  mergedLabel = new QLabel("0");
  hbl->addWidget(mergedLabel);

  addActions();

  nodeMenu->addAction(_navFirstPentagon);
  nodeMenu->addAction(_navNextPentagon);
  nodeMenu->addAction(_navPrevPentagon);
  nodeMenu->addAction(_labelBranches);
  nodeMenu->addAction(_showInfo);
  connect(_navFirstPentagon, SIGNAL(triggered()), this, SLOT(navFirstPentagon()));
  connect(_navNextPentagon, SIGNAL(triggered()), this, SLOT(navNextPentagon()));
  connect(_navPrevPentagon, SIGNAL(triggered()), this, SLOT(navPrevPentagon()));
  connect(_labelBranches, SIGNAL(triggered()), _tc, SLOT(labelBranches()));
  connect(_showInfo, SIGNAL(triggered()), _tc, SLOT(showNodeInfo()));


  analysisMenu = menuBar->addMenu(tr("&Analysis"));
  analysisMenu->addAction(_showPentagonHist);
  analysisMenu->addAction(_saveComparisonStats);
  connect(_showPentagonHist, SIGNAL(triggered()), this, SLOT(showPentagonHist()));
  connect(_saveComparisonStats, SIGNAL(triggered()), this, SLOT(saveComparisonStats()));

  /// sort the pentagons by nodes diff:




  comparison_.compare(tc1, tc2, _tc);

  comparison_.sortPentagons();

  mergedLabel->setNum(comparison_.get_no_pentagons());
  // statusChangedShared(true);

}

void
CmpTreeDialog::addActions(void) {
  /// Note(maxim): Qt::WindowShortcut is default context
  _navFirstPentagon = new QAction("To first pentagon", this);
  _navFirstPentagon->setShortcut(QKeySequence("Ctrl+Shift+1"));

  _navNextPentagon = new QAction("To next pentagon", this);
  _navNextPentagon->setShortcut(QKeySequence("Ctrl+Shift+Right"));

  _navPrevPentagon = new QAction("To prev pentagon", this);
  _navPrevPentagon->setShortcut(QKeySequence("Ctrl+Shift+Left"));

  _showPentagonHist = new QAction("Pentagon list", this);

  _saveComparisonStats = new QAction("Save comparison stats", this);

  _labelBranches = new QAction("Label/clear branches", this);
  _labelBranches->setShortcut(QKeySequence("L"));

  _showInfo = new QAction("Show info", this);
  _showInfo->setShortcut(QKeySequence("I"));

  addAction(_navFirstPentagon);
  addAction(_navNextPentagon);
  addAction(_navPrevPentagon);
  addAction(_labelBranches);
  addAction(_showInfo);
}

void
CmpTreeDialog::statusChanged(VisualNode*, const Statistics&, bool finished) {

  statusChangedShared(finished);

}

void
CmpTreeDialog::navFirstPentagon(void) {
  const std::vector<VisualNode*>& pentagon_nodes = comparison_.pentagon_nodes();

  if (pentagon_nodes.size() == 0) {
    qDebug() << "warning: pentagons.size() == 0";
    return;
  }

  _tc->setCurrentNode(pentagon_nodes[0]);
  _tc->centerCurrentNode();

}

void
CmpTreeDialog::navNextPentagon(void) {

  _tc->navNextPentagon();

}

void
CmpTreeDialog::navPrevPentagon(void) {

  _tc->navNextPentagon(true);
}

void
CmpTreeDialog::showPentagonHist(void) {
  pentListWindow.createList(comparison_.pentagon_nodes(), comparison_.pentagon_sizes());
  pentListWindow.show();
}

void
CmpTreeDialog::saveComparisonStatsTo(const QString& file_name) {
  if (file_name != "") {
        QFile outputFile(file_name);
        if (outputFile.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&outputFile);

            auto pentagons_diff = comparison_.pentagon_sizes();

            for (auto& pair : pentagons_diff) {
              out << pair.first << " " << pair.second << "\n";
            }

            qDebug() << "writing comp stats to the file: " << file_name;
        } else {
          qDebug() << "could not open the file: " << file_name;
        }
    }
}

void
CmpTreeDialog::saveComparisonStats(void) {
  saveComparisonStatsTo("/home/maxim/temp_stats.txt");
}

/// *************** Pentagon List Window ****************

PentListWindow::PentListWindow(QWidget* parent)
: QDialog(parent), _histTable{this} {

  resize(600, 400);

  connect(&_histTable, SIGNAL(cellDoubleClicked (int, int)), parent, SLOT(selectPentagon(int, int)));
  QHBoxLayout* layout = new QHBoxLayout(this);

  _histTable.setEditTriggers(QAbstractItemView::NoEditTriggers);
  _histTable.setSelectionBehavior(QAbstractItemView::SelectRows);

  layout->addWidget(&_histTable);
}

void
PentListWindow::createList(const std::vector<VisualNode*>& pentagon_nodes,
                           std::vector<std::pair<unsigned int, unsigned int>> pentagon_sizes)
{

  assert(pentagon_nodes.size() == pentagon_sizes.size());

  _histTable.setColumnCount(2);
  _histTable.setRowCount(pentagon_sizes.size());

  QStringList table_header;
  table_header << "Left" << "Right";
  _histTable.setHorizontalHeaderLabels(table_header);

  for (unsigned int i = 0; i < pentagon_sizes.size(); i++) {
    _histTable.setItem(i, 0, new QTableWidgetItem(QString::number(pentagon_sizes[i].first)));
    _histTable.setItem(i, 1, new QTableWidgetItem(QString::number(pentagon_sizes[i].second)));
  }

}

void
CmpTreeDialog::selectPentagon(int row, int) {
  const std::vector<VisualNode*>& pentagon_nodes = comparison_.pentagon_nodes();

  auto* node = pentagon_nodes[row];


  //TODO(maxim): this should unhide all nodes above
  // _tc->unhideNode(node); // <- does not work correctly
  _tc->setCurrentNode(node);
  _tc->centerCurrentNode();

  
}

/// ******************************************************

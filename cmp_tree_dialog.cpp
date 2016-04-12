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
#include "treecomparison.hh"
#include "treecanvas.hh"

#include "third-party/json.hpp"
#include <algorithm>
#include <cmath>

/// TODO(maxim): use normal Gist window for comparison instead???

CmpTreeDialog::CmpTreeDialog(QWidget* parent, Execution* execution, bool withLabels,
                             TreeCanvas* tc1, TreeCanvas* tc2)
    : BaseTreeDialog(parent, execution, CanvasType::MERGED),
      analysisMenu{nullptr} {

  hbl->addWidget(new NodeWidget(MERGING));
  mergedLabel = new QLabel("0");
  comparison_ = new TreeComparison{*tc1->getExecution(), *tc2->getExecution(), withLabels};
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

  auto listNogoodsAction = new QAction("List Responsible Nogoods", this);


  analysisMenu = menuBar->addMenu(tr("&Analysis"));
  analysisMenu->addAction(_showPentagonHist);
  analysisMenu->addAction(_saveComparisonStats);
  analysisMenu->addAction(listNogoodsAction);

  connect(_showPentagonHist, SIGNAL(triggered()), this, SLOT(showPentagonHist()));
  connect(_saveComparisonStats, SIGNAL(triggered()), this, SLOT(saveComparisonStats()));
  connect(listNogoodsAction, &QAction::triggered, this, &CmpTreeDialog::showResponsibleNogoods);

  /// sort the pentagons by nodes diff:

  comparison_->compare(_tc);

  comparison_->sortPentagons();

  mergedLabel->setNum(comparison_->get_no_pentagons());
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
  const auto pentagon_items = comparison_->pentagon_items();

  if (pentagon_items.size() == 0) {
    qDebug() << "warning: pentagons.size() == 0";
    return;
  }

  _tc->setCurrentNode(pentagon_items[0].node);
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

  auto pentagon_window = new PentListWindow(this, comparison_->pentagon_items());
  pentagon_window->createList();
  pentagon_window->show();
}

void
CmpTreeDialog::saveComparisonStatsTo(const QString& file_name) {
  if (file_name != "") {
        QFile outputFile(file_name);
        if (outputFile.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&outputFile);

            const auto pentagon_items = comparison_->pentagon_items();

            for (auto& item : pentagon_items) {
              out << item.l_size << " " << item.r_size << "\n";
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


void initNogoodTable(QTableWidget& ng_table) {
  ng_table.setEditTriggers(QAbstractItemView::NoEditTriggers);

  ng_table.setColumnCount(3);

  QStringList table_header;
  table_header << "Id" << "Occurrence" << "Literals";
  ng_table.setHorizontalHeaderLabels(table_header);
  ng_table.setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  ng_table.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);


}

std::vector<int>
infoToNogoodVector(const string& info) {
  auto info_json = nlohmann::json::parse(info);

  auto nogoods = info_json["nogoods"];

  if (nogoods.is_array()) {
    return nogoods;
  }

  return {};
}

void
PentListWindow::populateNogoodTable(const vector<int>& nogoods) {

  auto ng_counts = comparison_.responsible_nogood_counts();

  _nogoodTable.setRowCount(nogoods.size());

  for (unsigned int i = 0; i < nogoods.size(); i++) {

    int ng_id = nogoods[i]; /// is this sid of gid???
    _nogoodTable.setItem(i, 0, new QTableWidgetItem(QString::number(ng_id)));

    auto nogood_map = comparison_.left_execution().getNogoods();

    string nogood = "";

    auto maybe_nogood = nogood_map.find(ng_id);

    if (maybe_nogood != nogood_map.end()){
      nogood = maybe_nogood->second;
    }


    int ng_count = ng_counts.at(ng_id);

    _nogoodTable.setItem(i, 1, new QTableWidgetItem(QString::number(ng_count)));

    _nogoodTable.setItem(i, 2, new QTableWidgetItem(nogood.c_str()));

  }

  _nogoodTable.resizeColumnsToContents();

}

PentListWindow::PentListWindow(CmpTreeDialog* parent, const std::vector<PentagonItem>& items)
: QDialog(parent), _pentagonTable{this}, _items(items), comparison_(parent->comparison()) {

  resize(600, 400);

  setAttribute(Qt::WA_DeleteOnClose, true);

  connect(&_pentagonTable, &QTableWidget::cellDoubleClicked, [this, parent](int row, int) {
    static_cast<CmpTreeDialog*>(parent)->selectPentagon(row);

    auto maybe_info = _items[row].info;

    /// clear nogood view
    _nogoodTable.clearContents();

    if (maybe_info) {
      auto nogoods = infoToNogoodVector(*maybe_info);

      populateNogoodTable(nogoods);
    }
  });

  auto layout = new QVBoxLayout(this);

  _pentagonTable.setEditTriggers(QAbstractItemView::NoEditTriggers);
  _pentagonTable.setSelectionBehavior(QAbstractItemView::SelectRows);

  initNogoodTable(_nogoodTable);

  layout->addWidget(&_pentagonTable);
  layout->addWidget(&_nogoodTable);
}



QString infoToNogoodStr(const string& info) {
  QString result = "";

  auto info_json = nlohmann::json::parse(info);

  auto nogoods = info_json["nogoods"];

  if (nogoods.is_array() && nogoods.size() > 0) {
    for (auto nogood : nogoods) {
      int ng = nogood;
      result += QString::number(ng) + " ";
    }
  }

  return result;
}

void
PentListWindow::createList()
{


  _pentagonTable.setColumnCount(3);
  _pentagonTable.setRowCount(_items.size());

  QStringList table_header;
  table_header << "Left" << "Right" << "Nogoods involved";
  _pentagonTable.setHorizontalHeaderLabels(table_header);
  _pentagonTable.setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  _pentagonTable.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);


  for (unsigned int i = 0; i < _items.size(); i++) {
    _pentagonTable.setItem(i, 0, new QTableWidgetItem(QString::number(_items[i].l_size)));
    _pentagonTable.setItem(i, 1, new QTableWidgetItem(QString::number(_items[i].r_size)));

    auto maybe_info = _items[i].info;
    if (maybe_info) {

     QString nogood_str = infoToNogoodStr(*maybe_info);

      _pentagonTable.setItem(i, 2, new QTableWidgetItem(nogood_str));
    }
  }
  _pentagonTable.resizeColumnsToContents();

}

void
CmpTreeDialog::selectPentagon(int row) {
  const auto items = comparison_->pentagon_items();

  auto node = items[row].node;

  //TODO(maxim): this should unhide all nodes above
  // _tc->unhideNode(node); // <- does not work correctly
  _tc->setCurrentNode(node);
  _tc->centerCurrentNode();
  
}


void
CmpTreeDialog::showResponsibleNogoods() {

  auto ng_dialog = new QDialog(this);
  auto ng_layout = new QVBoxLayout();

  ng_dialog->resize(600, 400);
  ng_dialog->setLayout(ng_layout);
  ng_dialog->show();


  auto ng_table = new QTableWidget(ng_dialog);
  ng_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ng_table->setColumnCount(2);
  ng_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  ng_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  QStringList table_header;  
  table_header << "Id" << "Occurrence" << "Literals";
  ng_table->setHorizontalHeaderLabels(table_header);

  ng_layout->addWidget(ng_table);

  /// *** edit table ***

  auto ng_counts = comparison_->responsible_nogood_counts();

  /// map to vector
  std::vector<std::pair<int, int> > ng_counts_vector;
  ng_counts_vector.reserve(ng_counts.size());

  for (auto ng : ng_counts) {
    ng_counts_vector.push_back(ng);
  }

  std::sort(ng_counts_vector.begin(), ng_counts_vector.end(),
    [](const std::pair<int, int>& lhs, const std::pair<int, int>& rhs){
      return lhs.second > rhs.second;
  });

  ng_table->setRowCount(ng_counts.size());


  int row = 0;
  for (auto ng : ng_counts_vector) {

    ng_table->setItem(row, 0, new QTableWidgetItem(QString::number(ng.first)));
    ng_table->setItem(row, 1, new QTableWidgetItem(QString::number(ng.second)));

    ++row;
  }



}

/// ******************************************************

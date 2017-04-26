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

#include "nogood_dialog.hh"
#include "treecanvas.hh"
#include "data.hh"
#include "execution.hh"
#include <QDebug>
#include <QHBoxLayout>
#include <QStandardItemModel>

#include "third-party/json.hpp"

const int NogoodDialog::DEFAULT_WIDTH = 600;
const int NogoodDialog::DEFAULT_HEIGHT = 400;

NogoodDialog::NogoodDialog(
    QWidget* parent, TreeCanvas& tc, const std::vector<int>& selected_nodes,
    const std::unordered_map<int, std::string>& sid2nogood)
  : QDialog(parent), _tc(tc), _sid2nogood(sid2nogood) {
  enum NCols {SID_COL=0, NOGOOD_COL};
  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

  _model = new QStandardItemModel(0, 2, this);
  QStringList tableHeaders {"node id", "clause"};
  _model->setHorizontalHeaderLabels(tableHeaders);

  populateTable(selected_nodes);

  QList<NogoodProxyModel::Sorter> sorters {
      NogoodProxyModel::SORTER_INT, NogoodProxyModel::SORTER_NOGOOD};
  _nogoodTable = new NogoodTableView(this, _model, sorters, SID_COL, NOGOOD_COL);
  _nogoodTable->sortByColumn(SID_COL, Qt::SortOrder::AscendingOrder);
  _nogoodTable->horizontalHeader()->setStretchLastSection(true);

  connect(_nogoodTable, SIGNAL(doubleClicked(const QModelIndex&)), this,
          SLOT(selectNode(const QModelIndex&)));

  auto layout = new QVBoxLayout(this);
  layout->addWidget(_nogoodTable);

  const NameMap* nm = _tc.getExecution().getNameMap();
  if(nm != nullptr) {
    auto buttons = new QHBoxLayout(this);
    auto heatmapButton = new QPushButton("Heatmap");
    buttons->addWidget(heatmapButton);
    auto showExpressions = new QPushButton("Show/Hide Expressions");
    buttons->addWidget(showExpressions);
    layout->addLayout(buttons);

    _nogoodTable->connectHeatmapButton(heatmapButton, _tc.getExecution(), _tc);
    _nogoodTable->connectShowExpressionsButton(showExpressions, _tc.getExecution());
  }

  auto regex_layout = new QHBoxLayout();
  regex_layout->addWidget(new QLabel{"Nogood Regex (comma separated):"});
  auto regex_edit = new QLineEdit("");
  regex_layout->addWidget(regex_edit);
  _nogoodTable->connectFilters(regex_edit);
  layout->addLayout(regex_layout);
}

NogoodDialog::~NogoodDialog() {}

void NogoodDialog::populateTable(const std::vector<int>& selected_nodes) {
  int row = 0;
  auto sid2info = _tc.getExecution().getInfo();
  const NameMap* nm = _tc.getExecution().getNameMap();
  for (auto it = selected_nodes.begin(); it != selected_nodes.end(); it++) {
    int gid = *it;

    int64_t sid = _tc.getExecution().getData().gid2sid(gid);

    /// TODO(maxim): check if a node is a failure node

    auto ng_item = _sid2nogood.find(sid);
    if (ng_item == _sid2nogood.end()) {
      continue;  /// nogood not found
    }

    QString qclause = QString::fromStdString(ng_item->second);
    QString clause = nm != nullptr ? nm->replaceNames(qclause) : qclause;

    _model->setItem(row, 0, new QStandardItem(QString::number(sid)));
    _model->setItem(row, 1, new QStandardItem(clause));

    row++;
  }
}

void NogoodDialog::selectNode(const QModelIndex& index) {
  int gid = _tc.getExecution().getData().getGidBySid(index.sibling(index.row(), 0).data().toInt());
  _tc.navigateToNodeById(gid);
}

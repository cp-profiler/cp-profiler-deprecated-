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

const int NogoodDialog::DEFAULT_WIDTH = 600;
const int NogoodDialog::DEFAULT_HEIGHT = 400;

NogoodDialog::NogoodDialog(
    QWidget* parent, TreeCanvas& tc, const std::vector<int>& selected_nodes,
    const std::unordered_map<int, std::string>& sid2nogood, int64_t root_gid)
  : QDialog(parent), _tc(tc), _sid2nogood(sid2nogood), expand_expressions(false),
    _root_gid(root_gid) {
  int sid_col = 0;
  int nogood_col = 1;
  _model = new QStandardItemModel(0, 2, this);

  _nogoodTable = new QTableView(this);
  _nogoodTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _nogoodTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  _nogoodTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  _nogoodTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  QStringList tableHeaders;
  tableHeaders << "node id"
               << "clause";
  _model->setHorizontalHeaderLabels(tableHeaders);

  _nogoodTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  _nogoodTable->setSortingEnabled(true);
  _nogoodTable->horizontalHeader()->setStretchLastSection(true);

  connect(_nogoodTable, SIGNAL(doubleClicked(const QModelIndex&)), this,
          SLOT(selectNode(const QModelIndex&)));

  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

  auto layout = new QVBoxLayout(this);

  populateTable(selected_nodes);

  _nogoodTable->resizeColumnsToContents();
  _proxy_model = new MyProxyModel(this);
  _proxy_model->setSourceModel(_model);

  _nogoodTable->setModel(_proxy_model);
  _nogoodTable->setSortingEnabled(true);
  _nogoodTable->sortByColumn(sid_col, Qt::SortOrder::AscendingOrder);

  layout->addWidget(_nogoodTable);

  const NameMap* nm = _tc.getExecution().getNameMap();
  if(nm != nullptr) {
    auto heatmapButton = new QPushButton("Heatmap");
    connect(heatmapButton, &QPushButton::clicked, [=](){
      const QString heatmap = nm->getHeatMapFromModel(
            _tc.getExecution().getInfo(), *_nogoodTable, sid_col);
      if(!heatmap.isEmpty())
        _tc.emitShowNogoodToIDE(heatmap);
    });

    auto showExpressions = new QPushButton("Show/Hide Expressions");
    connect(showExpressions, &QPushButton::clicked, [=](){
      expand_expressions ^= true;
      nm->refreshModelRenaming(
            _tc.getExecution().getNogoods(), *_nogoodTable,
            sid_col, expand_expressions,
            [=](int row, QString newNogood) {
        QModelIndex idx = _proxy_model->mapToSource(_proxy_model->index(row, 0, QModelIndex()));
        _model->setItem(idx.row(), nogood_col, new QStandardItem(newNogood));
      });
    });

    auto buttons = new QHBoxLayout(this);
    buttons->addWidget(heatmapButton);
    buttons->addWidget(showExpressions);

    layout->addLayout(buttons);
  }

  auto row_filter_lo = new QHBoxLayout();
  layout->addLayout(row_filter_lo);

  auto filter_label = new QLabel{"Filter Nogoods (comma separated):"};
  row_filter_lo->addWidget(filter_label);

  auto filter_edit = new QLineEdit("");
  row_filter_lo->addWidget(filter_edit);

  connect(filter_edit, &QLineEdit::returnPressed, [this, filter_edit, nogood_col] () {
    const QStringList splitFilter = filter_edit->text().split(",");
    const QRegExp reg_filter("^(?=.*" + splitFilter.join(")(?=.*") + ").*$");
    _proxy_model->setFilterKeyColumn(nogood_col);
    _proxy_model->setFilterRegExp(reg_filter);
  });

  layout->addLayout(row_filter_lo);
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
    QString clause = nm != nullptr ? nm->replaceNames(qclause, expand_expressions) : qclause;

    _model->setItem(row, 0, new QStandardItem(QString::number(sid)));
    _model->setItem(row, 1, new QStandardItem(clause));

    row++;
  }
}

void NogoodDialog::selectNode(const QModelIndex& index) {
  int gid = _tc.getExecution().getData().getGidBySid(index.sibling(index.row(), 0).data().toInt());
  _tc.navigateToNodeById(gid);
}

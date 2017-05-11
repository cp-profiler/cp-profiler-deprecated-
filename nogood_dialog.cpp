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
#include <QHBoxLayout>
#include <QStandardItemModel>

const int NogoodDialog::DEFAULT_WIDTH = 600;
const int NogoodDialog::DEFAULT_HEIGHT = 400;

NogoodDialog::NogoodDialog(
    QWidget* parent, TreeCanvas& tc, const std::vector<int>& selected_nodes)
  : QDialog(parent), _tc(tc) {
  enum NCols {SID_COL=0, NOGOOD_COL};
  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

  _model = new QStandardItemModel(0, 2, this);
  QStringList tableHeaders {"node id", "clause"};
  _model->setHorizontalHeaderLabels(tableHeaders);

  populateTable(selected_nodes);

  QVector<NogoodProxyModel::Sorter> sorters {
      NogoodProxyModel::SORTER_SID, NogoodProxyModel::SORTER_NOGOOD};
  _nogoodTable = new NogoodTableView(this, _model, sorters,
                                     _tc.getExecution(),
                                     SID_COL, NOGOOD_COL);
  _nogoodTable->sortByColumn(SID_COL, Qt::SortOrder::AscendingOrder);
  _nogoodTable->horizontalHeader()->setStretchLastSection(true);

  connect(_nogoodTable, SIGNAL(doubleClicked(const QModelIndex&)),
          this,         SLOT(selectNode(const QModelIndex&)));

  auto layout = new QVBoxLayout(this);
  layout->addWidget(_nogoodTable);

  const NameMap* nm = _tc.getExecution().getNameMap();
  if(nm) {
    auto buttons = new QHBoxLayout(this);
    auto heatmapButton = new QPushButton("Heatmap");
    heatmapButton->setAutoDefault(false);
    auto showExpressions = new QPushButton("Show/Hide Expressions");
    showExpressions->setAutoDefault(false);
    auto getFlatZinc = new QPushButton("Get FlatZinc");
    getFlatZinc->setAutoDefault(false);

    buttons->addWidget(heatmapButton);
    buttons->addWidget(showExpressions);
    buttons->addWidget(getFlatZinc);
    layout->addLayout(buttons);

    _nogoodTable->connectHeatmapButton(heatmapButton, _tc);
    _nogoodTable->connectShowExpressionsButton(showExpressions);
    _nogoodTable->connectFlatZincButton(getFlatZinc);
  }

  auto filter_layout = new QHBoxLayout();
  filter_layout->addWidget(new QLabel{"Text Include:"});
  auto include_edit = new QLineEdit("");
  filter_layout->addWidget(include_edit);
  filter_layout->addWidget(new QLabel{"Omit:"});
  auto reject_edit = new QLineEdit("");
  filter_layout->addWidget(reject_edit);
  _nogoodTable->connectTextFilter(include_edit, reject_edit);

  if(nm) {
    filter_layout->addWidget(new QLabel{"Location Filter:"});
    auto location_edit = new QLineEdit("");
    _nogoodTable->connectLocationFilter(location_edit);
    filter_layout->addWidget(location_edit);

    auto locationButton = new QPushButton("Get Location");
    locationButton->setAutoDefault(false);
    _nogoodTable->connectLocationButton(locationButton, location_edit);
    filter_layout->addWidget(locationButton);
  }
  layout->addLayout(filter_layout);

  auto subsumlayout = new QHBoxLayout();
  auto subsumbutton = new QPushButton("Simplify nogoods");
  subsumbutton->setAutoDefault(false);
  _nogoodTable->connectSubsumButton(subsumbutton);
  subsumlayout->addWidget(subsumbutton);

  layout->addLayout(subsumlayout);
}

NogoodDialog::~NogoodDialog() {}

void NogoodDialog::populateTable(const std::vector<int>& selected_nodes) {
  int row = 0;
  const NameMap* nm = _tc.getExecution().getNameMap();
  for (auto it = selected_nodes.begin(); it != selected_nodes.end(); it++) {
    int gid = *it;
    int64_t sid = _tc.getExecution().getData().gid2sid(gid);
    QString qclause = QString::fromStdString(_tc.getExecution().getNogoodBySid(sid));
    if(!qclause.isEmpty()) {
      QString clause = nm != nullptr ? nm->replaceNames(qclause) : qclause;

      _model->setItem(row, 0, new QStandardItem(QString::number(sid)));
      _model->setItem(row, 1, new QStandardItem(clause));

      row++;
    }
  }
}

void NogoodDialog::selectNode(const QModelIndex& index) {
  int gid = _tc.getExecution().getData().getGidBySid(index.sibling(index.row(), 0).data().toInt());
  _tc.navigateToNodeById(gid);
}

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
#include <QDebug>
#include <QHBoxLayout>
#include <QStandardItemModel>


const int NogoodDialog::DEFAULT_WIDTH = 600;
const int NogoodDialog::DEFAULT_HEIGHT = 400;

NogoodDialog::NogoodDialog(QWidget* parent, TreeCanvas& tc,
    const std::vector<int>& selected_nodes,
    const std::unordered_map<unsigned long long, std::string>& sid2nogood)
: QDialog(parent), _tc(tc), _sid2nogood(sid2nogood) {

  _model = new QStandardItemModel(0,2,this);


  _nogoodTable = new QTableView(this);
  _nogoodTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _nogoodTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  QStringList tableHeaders; tableHeaders << "node id" << "clause";
  _model->setHorizontalHeaderLabels(tableHeaders);


  _nogoodTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  _nogoodTable->setSortingEnabled(true);
  _nogoodTable->horizontalHeader()->setStretchLastSection(true);

  connect(_nogoodTable, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(selectNode(const QModelIndex&)));

  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

  QHBoxLayout* layout = new QHBoxLayout(this);

  populateTable(selected_nodes);

  _proxy_model = new MyProxyModel(this);
  _proxy_model->setSourceModel(_model);

  _nogoodTable->setModel(_proxy_model);


  layout->addWidget(_nogoodTable);

}

NogoodDialog::~NogoodDialog() {

}

void NogoodDialog::populateTable(const std::vector<int>& selected_nodes) {

  int row = 0;
  for (auto it = selected_nodes.begin(); it != selected_nodes.end(); it++) {

      unsigned long long sid = _tc.getExecution()->getData()->gid2sid(*it);


    auto ng_item = _sid2nogood.find(sid);
    if (ng_item == _sid2nogood.end()) continue; /// nogood not found

    _model->setItem(row, 0, new QStandardItem(QString::number(sid)));
    _model->setItem(row, 1, new QStandardItem((ng_item->second).c_str()));
    row++;

  }

  _nogoodTable->resizeColumnsToContents();
}

void NogoodDialog::selectNode(const QModelIndex & index) {

  unsigned int sid = index.sibling(index.row(), 0).data().toInt();
  _tc.navigateToNodeBySid(sid); /// assuming that parent is TreeCanvas
}

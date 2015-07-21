#include "nogood_dialog.hh"
#include "treecanvas.hh"
#include <QDebug>
#include <QHBoxLayout>


const int NogoodDialog::DEFAULT_WIDTH = 600;
const int NogoodDialog::DEFAULT_HEIGHT = 400;

NogoodDialog::NogoodDialog(QWidget* parent, TreeCanvas& tc,
    const std::unordered_map<unsigned long long, std::string>& sid2nogood)
: QDialog(parent), _tc(tc), _sid2nogood(sid2nogood) {

  _nogoodTable = new QTableView(this);
  _model = new QStandardItemModel(0,2,this);
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

  populateTable();

  _nogoodTable->setModel(_model);

  layout->addWidget(_nogoodTable);

}

NogoodDialog::~NogoodDialog() {

}

void NogoodDialog::populateTable() {

  int row = 0;
  for (auto it = _sid2nogood.begin(); it != _sid2nogood.end(); it++) {
    _model->setItem(row, 0, new QStandardItem(QString::number(it->first)));
    _model->setItem(row, 1, new QStandardItem(it->second.c_str()));
    row++;

  }

  _nogoodTable->resizeColumnsToContents();
}

void NogoodDialog::selectNode(const QModelIndex & index) {

  unsigned int sid = _model->data(_model->index(index.row(), 0)).toInt();
  _tc.navigateToNodeBySid(sid); /// assuming that parent is TreeCanvas
}
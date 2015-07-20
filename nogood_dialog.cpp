#include "nogood_dialog.hh"
#include "treecanvas.hh"
#include <QDebug>
#include <QHBoxLayout>


const int NogoodDialog::DEFAULT_WIDTH = 600;
const int NogoodDialog::DEFAULT_HEIGHT = 400;

NogoodDialog::NogoodDialog(QWidget* parent, TreeCanvas& tc,
    const std::unordered_map<unsigned long long, std::string>& sid2nogood)
: QDialog(parent), _tc(tc), _sid2nogood(sid2nogood) {

  _nogoodTable = new QTableWidget(this);
  _nogoodTable->setColumnCount(2);
  _nogoodTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _nogoodTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  QStringList tableHeaders; tableHeaders << "node id" << "clause";
  _nogoodTable->setHorizontalHeaderLabels(tableHeaders);
  _nogoodTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  connect(_nogoodTable, SIGNAL(cellDoubleClicked (int, int)), this, SLOT(selectNode(int, int)));

  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

  QHBoxLayout* layout = new QHBoxLayout(this);

  populateTable();

  layout->addWidget(_nogoodTable);

}

NogoodDialog::~NogoodDialog() {

}

void NogoodDialog::populateTable() {
  _nogoodTable->setRowCount(_sid2nogood.size());

  int row = 0;
  for (auto it = _sid2nogood.begin(); it != _sid2nogood.end(); it++) {
    // qDebug() << it->first;
    row2sid.push_back(it->first);
    _nogoodTable->setItem(row, 0, new QTableWidgetItem(QString::number(it->first)));
    _nogoodTable->setItem(row, 1, new QTableWidgetItem(it->second.c_str()));
    row++;

  }

  _nogoodTable->resizeColumnsToContents();
}

void NogoodDialog::selectNode(int row, int column) {
  unsigned int sid = row2sid[row];
  _tc.navigateToNodeBySid(sid); /// assuming that parent is TreeCanvas
}
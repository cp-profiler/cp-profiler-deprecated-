#include "shape_aggregation.hh"
#include <map>
#include <algorithm>

#include <QDebug>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>

using std::string;
using std::pair;

namespace cpprofiler {
namespace analysis {

ShapeAggregationWin::ShapeAggregationWin(std::vector<string>&& vec) {

  std::map<string, int> labels_map;

  for (auto& l : vec) {
    labels_map[l]++;
  }

  std::vector<LCount> label_counts;

  for (auto& pair: labels_map) {
    label_counts.push_back(pair);
  }

  qDebug() << "size of label_counts: " << label_counts.size();

  std::sort(begin(label_counts), end(label_counts),
            [](const LCount& lhs, const LCount& rhs) {
              return lhs.second > rhs.second;
            });

  show();

  for (auto& pair : label_counts) {
    qDebug() << pair.first.c_str() << ":\t" << pair.second;
  }

  auto layout = new QHBoxLayout{this};

  dataTable = new QTableView{this};
  dataTable->horizontalHeader()->setStretchLastSection(true);
  dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  layout->addWidget(dataTable);

  dataModel = new QStandardItemModel{0, 2, this};
  QStringList tableHeaders{"label", "count"};
  dataModel->setHorizontalHeaderLabels(tableHeaders);

  dataTable->setModel(dataModel);

  populateTable(label_counts);
}

void ShapeAggregationWin::populateTable(const std::vector<LCount>& vec) {

  for (auto& lcount: vec) {

    auto label_item = new QStandardItem{lcount.first.c_str()};
    auto count_item = new QStandardItem{QString::number(lcount.second)};

    dataModel->appendRow({label_item, count_item});
  }
}



}}
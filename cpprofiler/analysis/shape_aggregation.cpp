#include "shape_aggregation.hh"
#include "globalhelper.hh"
#include <map>
#include <algorithm>

#include <QDebug>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>

using std::string;
using std::pair;

namespace cpprofiler {
namespace analysis {

ShapeAggregationWin::ShapeAggregationWin(std::vector<string>&& vec) {

  resize(600, 400);

  std::map<string, int> labels_map;

  for (auto& l : vec) {
    labels_map[l]++;
  }

  for (auto& pair: labels_map) {
    label_counts.push_back(pair);
  }

  std::sort(begin(label_counts), end(label_counts),
            [](const LCount& lhs, const LCount& rhs) {
              return lhs.second > rhs.second;
            });

  auto layout = new QVBoxLayout{this};

  dataTable = new QTableView{this};
  dataTable->horizontalHeader()->setStretchLastSection(true);
  dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  layout->addWidget(dataTable);

  auto vars_only_cb = new QCheckBox{"Variables only"};
  layout->addWidget(vars_only_cb);

  connect(vars_only_cb, &QCheckBox::stateChanged, [this](int state) {
    settings.vars_only = (state == Qt::Checked);
    update();
  });

  dataModel = new QStandardItemModel{0, 2, this};
  QStringList tableHeaders{"label", "count"};
  dataModel->setHorizontalHeaderLabels(tableHeaders);

  dataTable->setModel(dataModel);

  update();
  show();
}


static std::string extract_var(const std::string& label) {

  auto found = findAnyOf(label, "=", "!=", "<", ">", ">=", "=<");

  if (found != std::string::npos) {
    return label.substr(0, found);
  }

  qDebug() << "can't extract var from a label";
  return label;
}

void ShapeAggregationWin::update() {

  if (!settings.vars_only) {
    populateTable(label_counts);
    return;
  }

  std::map<string, int> var_count_map;

  for (auto& pair: label_counts) {
    const auto& var = extract_var(pair.first);
    var_count_map[var] += pair.second;
  }

  std::vector<LCount> var_count;

  for (auto& pair : var_count_map) {
    var_count.push_back(pair);
  }

  std::sort(begin(var_count), end(var_count),
            [](const LCount& lhs, const LCount& rhs) {
              return lhs.second > rhs.second;
            });

  populateTable(var_count);

}

void ShapeAggregationWin::populateTable(const std::vector<LCount>& vec) {

  dataModel->removeRows(0, dataModel->rowCount());

  for (auto& lcount: vec) {

    auto label_item = new QStandardItem{lcount.first.c_str()};
    auto count_item = new QStandardItem{QString::number(lcount.second)};

    dataModel->appendRow({label_item, count_item});
  }
}



}}
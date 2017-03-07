#pragma once

#include <QDialog>

class QTableView;
class QStandardItemModel;

namespace cpprofiler {
namespace analysis {

using LCount = std::pair<std::string, int>;

/// Input info requred:
/// array of nodes to figure out diff variables
/// label

class ShapeAggregationWin : public QDialog {

private:

  QTableView* dataTable;
  QStandardItemModel* dataModel;

  void populateTable(const std::vector<LCount>& vec);

public:
  ShapeAggregationWin(std::vector<std::string>&& vec);
};


}}
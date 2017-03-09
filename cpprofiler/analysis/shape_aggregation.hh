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

  struct Settings {
    bool vars_only {false};
  };

private:

  QTableView* dataTable;
  QStandardItemModel* dataModel;

  Settings settings;

  std::vector<LCount> label_counts;

  void populateTable(const std::vector<LCount>& vec);
  void update();

public:
  ShapeAggregationWin(std::vector<std::string>&& vec);
};


}}
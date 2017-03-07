#pragma once

#include <QDialog>

namespace cpprofiler {
namespace analysis {


/// Input info requred:
/// array of nodes to figure out diff variables
/// label

class ShapeAggregationWin : public QDialog {

public:
  ShapeAggregationWin(std::vector<std::string>&& vec);
};


}}
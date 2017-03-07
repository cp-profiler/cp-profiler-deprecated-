#include "shape_aggregation.hh"
#include <QDebug>
#include <map>
#include <algorithm>

using std::string;
using std::pair;

namespace cpprofiler {
namespace analysis {


using LCount = pair<string, int>;

ShapeAggregationWin::ShapeAggregationWin(std::vector<string>&& vec) {

  std::map<string, int> labels_map;

  for (auto& l : vec) {
    labels_map[l]++;
  }

  std::vector<LCount> label_counts;

  for (auto& pair: label_counts) {
    label_counts.push_back(pair);
  }

  qDebug() << "size of label_counts: " << label_counts.size();

  std::sort(begin(label_counts), end(label_counts),
            [](const LCount& lhs, const LCount& rhs) {
              return lhs.second > rhs.second;
            });

  for (auto& pair : label_counts) {
    qDebug() << pair.first.c_str() << ":\t" << pair.second;
  }
}



}}
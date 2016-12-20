#pragma once

#ifndef SUBTREE_COMP_WIN_HH
#define SUBTREE_COMP_WIN_HH


#include <vector>
#include <memory>
#include <unordered_map>

#include "histogram_win.hh"
#include "subtree_analysis.hh"

class NodeTree;
class VisualNode;

class QGraphicsScene;
class QGraphicsView;
class QAbstractScrollArea;

using ExecMap_t = std::unordered_map<VisualNode*, int>;

namespace cpprofiler {
namespace analysis {

enum class ShapeProperty2 { SIZE, COUNT, CC, HEIGHT };

class SubtreeCanvas;

class Filters {
public:
    int minDepth = 2;
    int minCount = 2;

    bool check(int depth, int count) const {
      if (depth < minDepth) return false;
      if (count < minCount) return false;
      return true;
    }

};

class SubtreeCompWindow : public HistogramWindow {
  Q_OBJECT

    /// the result of identical subrees analysis
  std::unique_ptr<ExecMap_t> node2ex_id;

  Filters filters;

  ShapeProperty2   m_histType  = ShapeProperty2::SIZE;
  ShapeProperty2   m_sortType  = ShapeProperty2::SIZE;

  void initInterface();
  void updateHistogram();
  void drawComparisonHistogram();
public:
  SubtreeCompWindow(Execution& ex, std::unique_ptr<ExecMap_t>&& map);
  ~SubtreeCompWindow();

};




}}

#endif
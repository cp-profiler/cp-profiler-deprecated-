#include "histogram_win.hh"
#include "nodetree.hh"
#include "execution.hh"
#include "subtree_canvas.hh"
#include "cpprofiler/analysis/similar_shapes.hh"
#include "cpprofiler/utils/tree_utils.hh"
#include "similar_shape_algorithm.hh"

#include <QAbstractScrollArea>
#include <QGraphicsView>

#include <QLabel>
#include <QScrollBar>
#include <QPaintEvent>
#include <QGraphicsRectItem>

#include <QGraphicsView>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QSplitter>
#include <QSpinBox>

#include <algorithm>
#include <map>

using std::vector;
using std::string;
using std::map;

namespace cpprofiler {
namespace analysis {

HistogramWindow::HistogramWindow(Execution& ex)
    : execution{ex}, node_tree{ex.nodeTree()} {

    auto sa = new QAbstractScrollArea();
    initSharedInterface(sa);
    m_SubtreeCanvas.reset(new SubtreeCanvas(sa, node_tree));

    m_SubtreeCanvas->show();
}

HistogramWindow::~HistogramWindow() = default;

void HistogramWindow::handleRectClick(const ShapeRect* rect) {

  auto* subtree_info = rect_to_si.at(rect).get();

  highlightSubtrees(subtree_info->nodes[0]);
  workoutLabelDiff(subtree_info);
}

void HistogramWindow::highlightSubtrees(VisualNode* node) {

    // /// TODO(maxim): does this do the right thing if SUBTREE?
    m_SubtreeCanvas->showSubtree(node);

    execution.compareDomains();

    GroupsOfNodes_t* groups = nullptr;

    if (simType == SimilarityType::SHAPE) {
      groups = &shapes;
    } else if (simType == SimilarityType::SUBTREE) {
      groups = &m_identicalGroups;
    }

    /// find the right group
    for (auto& group : *groups) {
      bool found = false;
      for (auto n : group) {
        if (node == n) {
          found = true;
          break;
        }
      }

      if (!found) continue;

      utils::highlightSubtrees(node_tree, group, settings.hideNotHighlighted);
      break;
    }

}


static std::vector<std::string> pathLabels(const VisualNode* const node,
                                    const Execution& ex) {

  auto& na = ex.nodeTree().getNA();
  auto* cur_node = node;

  std::vector<std::string> labels;

  do {
    /// NOTE(maxim): for root nodes labels will be empty;
    /// they will be ignored (a bit of a hack)
    auto&& label = ex.getLabel(*cur_node);
    if (label != "") {
      labels.push_back(label);
    }
  } while (cur_node = cur_node->getParent(na));

  return labels;
}

template<typename T>
static vector<T> set_intersect(vector<T> v1, vector<T> v2) {

  /// set_intersection requires this to be at least
  /// as large as the smallest of the two sets
  vector<T> res(std::min(v1.size(), v2.size()));

  std::sort(begin(v1), end(v1));
  std::sort(begin(v2), end(v2));

  auto it = std::set_intersection(begin(v1), end(v1), begin(v2), end(v2),
                                  begin(res));

  res.resize(it - begin(res));

  return res;
}

template<typename T>
static vector<T> set_symmetric_diff(vector<T> v1, vector<T> v2) {

  vector<T> res(v1.size() + v2.size());

  std::sort(begin(v1), end(v1));
  std::sort(begin(v2), end(v2));

  auto it = std::set_symmetric_difference(begin(v1), end(v1), begin(v2), end(v2),
                                  begin(res));

  res.resize(it - begin(res));

  return res;
}

VecPair<string> getLabelDiff(const Execution& ex, const VisualNode* n1,
                             const VisualNode* n2) {
  auto path_1 = pathLabels(n1, ex);
  auto path_2 = pathLabels(n2, ex);

  auto diff = set_symmetric_diff(path_1, path_2);

  /// unique labels in 1
  vector<string> unique_1 = set_intersect(path_1, diff);
  /// unique labels in 2
  vector<string> unique_2 = set_intersect(path_2, diff);

  return std::make_pair(std::move(unique_1), std::move(unique_2));
}

vector<string> getLabelDiff(const Execution& ex,
                                 const std::vector<VisualNode*>& vec) {
  map<string, int> label_counts;

  /// count all labels
  for (auto node : vec) {
    auto path = pathLabels(node, ex);

    for (auto label : path) {
      label_counts[label]++;
    }

  }

  /// keep if the count is less than the paths count
  vector<string> result;

  for (auto& pair : label_counts) {
    if (pair.second != vec.size()) {
      result.push_back(pair.first);
    }
  }

  return result;
}

void HistogramWindow::workoutLabelDiff(const SubtreeInfo* const si) {

  /// NOTE(maxim): working with just two for now
  if (si->nodes.size() != 2) return;

  auto vec_pair = getLabelDiff(execution, si->nodes[0], si->nodes[1]);

  std::string text;

  for (auto& label : vec_pair.first) {
    text += label + " ";
  }

  text += "| ";

  for (auto& label : vec_pair.second) {
    text += label + " ";
  }

  labelDiff.setText(text.c_str());
}

void HistogramWindow::initSharedInterface(QAbstractScrollArea* sa) {
  m_scene.reset(new QGraphicsScene{});

  hist_view = new QGraphicsView{this};
  hist_view->setScene(m_scene.get());
  hist_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  sa->setAutoFillBackground(true);

  settingsLayout = new QHBoxLayout{};

  auto splitter = new QSplitter{this};

  splitter->addWidget(hist_view);
  splitter->addWidget(sa);
  splitter->setSizes(QList<int>{1, 1});  // for splitter to be centered

  filtersLayout = new QHBoxLayout{};
  miscLayout = new QHBoxLayout{};

  labelDiff.setReadOnly(true);

  auto globalLayout = new QVBoxLayout{this};
  globalLayout->addLayout(settingsLayout);
  globalLayout->addWidget(splitter, 1);
  globalLayout->addWidget(&labelDiff);
  globalLayout->addLayout(filtersLayout);
  globalLayout->addLayout(miscLayout);

#ifndef MAXIM_THESIS
#ifdef MAXIM_DEBUG
  globalLayout->addWidget(&debug_label);
#endif
#endif

}


}
}
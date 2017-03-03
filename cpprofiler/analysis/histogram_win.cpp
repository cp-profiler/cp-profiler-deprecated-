#include "histogram_win.hh"
#include "nodetree.hh"
#include "execution.hh"
#include "subtree_canvas.hh"
#include "cpprofiler/analysis/similar_shapes.hh"
#include "tree_utils.hh"
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

using std::vector;
using std::string;

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

void HistogramWindow::highlightSubtrees(VisualNode* node) {

    // /// TODO(maxim): does this do the right thing if SUBTREE?
    m_SubtreeCanvas->showSubtree(node);

    const auto& na = node_tree.getNA();
    auto root = node_tree.getRoot();

    if (simType == SimilarityType::SHAPE) {

      auto shape_it = std::find_if(begin(shapes), end(shapes), [node] (const ShapeInfo& si) {

        auto res = std::find(begin(si.nodes), end(si.nodes), node);

        return res != end(si.nodes);
      });

      if (shape_it != end(shapes)) {
        tree_utils::highlightSubtrees(node_tree, shape_it->nodes, settings.hideNotHighlighted);

      }
      // perfHelper.end();
    } else if (simType == SimilarityType::SUBTREE) {

      /// find the right group
      for (auto& group : m_identicalGroups) {
        bool found = false;
        for (auto n : group) {
          if (node == n) {
            found = true;
            break;
          }
        }

        if (!found) continue;

        std::vector<VisualNode*> vec;
        vec.reserve(group.size());

        for (auto n : group) {
          vec.push_back(n);
        }

        tree_utils::highlightSubtrees(node_tree, vec, settings.hideNotHighlighted);
        break;
      }

    }

}


std::vector<std::string> pathLabels(const VisualNode* const node,
                                    const Execution& ex) {

  auto& na = ex.nodeTree().getNA();
  auto* cur_node = node;

  std::vector<std::string> labels;

  do {
    labels.push_back(ex.getLabel(*cur_node));
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

void HistogramWindow::workoutLabelDiff(const ShapeRect* rect) {

  auto* subtree_info = rect_to_si.at(rect).get();

  std::string text;

  /// NOTE(maxim): working with two for now
  if (subtree_info->nodes.size() < 2) return;

  auto path_1 = pathLabels(subtree_info->nodes[0], execution);
  auto path_2 = pathLabels(subtree_info->nodes[1], execution);

  auto diff = set_symmetric_diff(path_1, path_2);

  /// unique labels in 1
  vector<string> unique_1 = set_intersect(path_1, diff);
  /// unique labels in 2
  vector<string> unique_2 = set_intersect(path_2, diff);

  for (auto& label : unique_1) {
    text += label + " ";
  }

  text += "| ";

  for (auto& label : unique_2) {
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

#ifdef MAXIM_DEBUG
  globalLayout->addWidget(&debug_label);
#endif

}


}
}
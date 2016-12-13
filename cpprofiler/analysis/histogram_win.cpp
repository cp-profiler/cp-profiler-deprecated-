#include "histogram_win.hh"
#include "nodetree.hh"
#include "subtree_canvas.hh"
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

namespace cpprofiler {
namespace analysis {


HistogramWindow::HistogramWindow(NodeTree& nt): node_tree{nt} {

    std::unique_ptr<QAbstractScrollArea> sa{new QAbstractScrollArea()};
    initSharedInterface(sa.get());
    m_SubtreeCanvas.reset(new SubtreeCanvas(std::move(sa), node_tree));

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
        tree_utils::highlightSubtrees(node_tree, shape_it->nodes);

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

        tree_utils::highlightSubtrees(node_tree, vec);
        break;
      }

    }

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

  auto globalLayout = new QVBoxLayout{this};
  globalLayout->addLayout(settingsLayout);
  globalLayout->addWidget(splitter, 1);
  globalLayout->addLayout(filtersLayout);

#ifdef MAXIM_DEBUG
  globalLayout->addWidget(&debug_label);
#endif

}


}
}
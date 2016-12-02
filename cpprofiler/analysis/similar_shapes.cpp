#include "similar_shapes.hh"

#include <algorithm>
#include <chrono>

#include <QLabel>
#include <QScrollBar>
#include <QPaintEvent>
#include <QGraphicsRectItem>
#include <QAbstractScrollArea>
#include <QGraphicsView>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QSplitter>
#include <QSpinBox>

#include "visualnode.hh"
#include "libs/perf_helper.hh"
#include "nodetree.hh"
#include "globalhelper.hh"
#include "tree_utils.hh"

#include "subtree_canvas.hh"
#include "shape_rect.hpp"
#include "similar_shapes_algorithm.hpp"
#include "identical_shapes.hpp"
#include "subsumed_subtrees.hpp"

using std::vector;

namespace cpprofiler {
namespace analysis {

/// TODO(maxim): show all subtrees of a particular shape

static ShapeProperty interpretShapeProperty(const QString& str) {
  if (str == "size") return ShapeProperty::SIZE;
  if (str == "count") return ShapeProperty::COUNT;
  if (str == "height") return ShapeProperty::HEIGHT;
  abort();
  return {};
}

SimilarShapesWindow::SimilarShapesWindow(NodeTree& nt)
    : node_tree{nt}, m_type{AnalysisType::SINGLE} {

  std::unique_ptr<QAbstractScrollArea> sa{new QAbstractScrollArea()};
  initInterface(sa.get());
  m_SubtreeCanvas.reset(new SubtreeCanvas(std::move(sa), node_tree));

  m_SubtreeCanvas->show();

  updateHistogram();
}

SimilarShapesWindow::~SimilarShapesWindow() = default;

SimilarShapesWindow::SimilarShapesWindow(
    NodeTree& nt, std::unique_ptr < std::unordered_map<VisualNode*, int>> map)
  : node_tree{nt}, m_type{AnalysisType::COMPARISON}, node2ex_id{std::move(map)}  {

    /// TODO(maxim): remove duplication
    std::unique_ptr<QAbstractScrollArea> sa{new QAbstractScrollArea()};
    initInterface(sa.get());
    m_SubtreeCanvas.reset(new SubtreeCanvas(std::move(sa), node_tree));

    m_SubtreeCanvas->show();

    updateHistogram();
  }

void SimilarShapesWindow::updateShapesData() {

  perfHelper.begin("shapes: analyse");
  shapes = runSimilarShapes(node_tree);
  perfHelper.end();

  perfHelper.begin("subsumed shapes elimination");
  eliminateSubsumed(node_tree, shapes);
  perfHelper.end();
}

void SimilarShapesWindow::highlightSubtrees(VisualNode* node) {

  /// TODO(maxim): does this do the right thing if SUBTREE?
  m_SubtreeCanvas->showSubtree(node);

  const auto& na = node_tree.getNA();
  auto root = node_tree.getRoot();

  if (simType == SimilarityType::SHAPE) {

    auto shape_it = std::find_if(begin(shapes), end(shapes), [node] (const ShapeInfo& si) {

      auto res = std::find(begin(si.nodes), end(si.nodes), node);

      return res != end(si.nodes);
    });

    // perfHelper.begin("actually highlighting");
    /// TODO(maxim): slow, run this in a parallel thread?
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

void SimilarShapesWindow::initInterface(QAbstractScrollArea* sa) {
  m_scene.reset(new QGraphicsScene{});

  hist_view = new QGraphicsView{this};
  hist_view->setScene(m_scene.get());
  hist_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  // sa->setAutoFillBackground(true);

  auto settingsLayout = new QHBoxLayout{};
  settingsLayout->addWidget(new QLabel{"Type:"});

  auto typeChoice = new QComboBox();
  typeChoice->addItem("shape");
  typeChoice->addItem("subtree");
  settingsLayout->addWidget(typeChoice);

  auto labels_flag = new QCheckBox{"Compare labels"};
  settingsLayout->addWidget(labels_flag);

  connect(labels_flag, &QCheckBox::stateChanged, [this](int state) {
      labelSensitive = (state == Qt::Checked);
      subtrees_cached = false;
      updateHistogram();
  });

  connect(typeChoice, &QComboBox::currentTextChanged, [this](const QString& str) {
    if (str == "shape") {
        simType = SimilarityType::SHAPE;
    } else if (str == "subtree") {
        simType = SimilarityType::SUBTREE;
    }
    updateHistogram();
  });

  auto splitter = new QSplitter{this};

  splitter->addWidget(hist_view);
  splitter->addWidget(sa);
  splitter->setSizes(QList<int>{1, 1});  // for splitter to be centered

  auto depthFilterSB = new QSpinBox{this};
  depthFilterSB->setMinimum(1);
  depthFilterSB->setValue(2);

  connect(depthFilterSB, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this] (int val) {
    filters.setMinDepth(val);
    updateHistogram();
  });

  auto countFilterSB = new QSpinBox{this};
  countFilterSB->setMinimum(1);
  countFilterSB->setValue(2);

  connect(countFilterSB, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int val) {
    filters.setMinCount(val);
    updateHistogram();
  });

  auto histChoiceCB = new QComboBox();
  histChoiceCB->addItem("size");
  histChoiceCB->addItem("count");
  histChoiceCB->addItem("height");
  connect(histChoiceCB, &QComboBox::currentTextChanged,
          [this](const QString& str) {
            m_histType = interpretShapeProperty(str);
            updateHistogram();
          });

  auto sortChoiceCB = new QComboBox();
  sortChoiceCB->addItem("size");
  sortChoiceCB->addItem("count");
  sortChoiceCB->addItem("height");
  connect(sortChoiceCB, &QComboBox::currentTextChanged,
          [this](const QString& str) {
            m_sortType = interpretShapeProperty(str);
            updateHistogram();
          });

  auto filtersLayout = new QHBoxLayout{};

  filtersLayout->addWidget(new QLabel("min height"));
  filtersLayout->addWidget(depthFilterSB);

  filtersLayout->addWidget(new QLabel("min count"));
  filtersLayout->addWidget(countFilterSB);

  filtersLayout->addStretch();

  filtersLayout->addWidget(new QLabel{"sort by: "});
  filtersLayout->addWidget(sortChoiceCB);

  filtersLayout->addWidget(new QLabel{"histogram: "});
  filtersLayout->addWidget(histChoiceCB);

  auto globalLayout = new QVBoxLayout{this};
  globalLayout->addLayout(settingsLayout);
  globalLayout->addWidget(splitter, 1);
  globalLayout->addLayout(filtersLayout);

#ifdef MAXIM_DEBUG
  globalLayout->addWidget(&debug_label);
#endif
}

/// TODO(maxim): see if any of these could be reused in tree comparison
///              (or vice versa)
namespace detail {

static bool areShapesIdentical(const NodeTree& nt,
                               const std::vector<VisualNode*>& nodes) {
  auto first_node = nodes[0];

  if (nodes.size() == 1) return true;

  for (auto it = ++begin(nodes); it < end(nodes); ++it) {
    auto equal = tree_utils::compareSubtrees(nt, *first_node, **it);
    if (!equal) return false;
  }
  return true;
}

}



/// Find a subtree from `vec` with the maximal `ShapeProperty` value and return that value
template<typename SI>
static int maxShapeValue(const std::vector<SI>& vec, ShapeProperty prop) {
  if (prop == ShapeProperty::SIZE) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.size < s2.size;
        });
    return res.size;
  } else if (prop == ShapeProperty::COUNT) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.get_count() < s2.get_count();
        });
    return res.get_count();
  } else if (prop == ShapeProperty::HEIGHT) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.height < s2.height;
        });
    return res.height;
  }

  return 1;
}

/// Sort elements of `vec` in place based on `prop`
static void sortSubtrees(std::vector<SubtreeInfo>& vec, ShapeProperty prop) {
  if (prop == ShapeProperty::SIZE) {
    std::sort(begin(vec), end(vec),
              [](const SubtreeInfo& s1, const SubtreeInfo& s2) {
                return s1.size > s2.size;
              });
  } else if (prop == ShapeProperty::COUNT) {
    std::sort(begin(vec), end(vec),
              [](const SubtreeInfo& s1, const SubtreeInfo& s2) {
                return s1.count > s2.count;
              });
  } else if (prop == ShapeProperty::HEIGHT) {
    std::sort(begin(vec), end(vec),
              [](const SubtreeInfo& s1, const SubtreeInfo& s2) {
                return s1.height > s2.height;
              });
  }
}

namespace detail {

inline void addText(QGraphicsSimpleTextItem* text_item, int x, int y,
                    QGraphicsScene& scene) {
  // center the item vertically at y
  int text_y_offset = text_item->boundingRect().height() / 2;
  text_item->setPos(x, y - text_y_offset);
  scene.addItem(text_item);
}
};

inline void addText(const char* text, int x, int y, QGraphicsScene& scene) {
  auto str_text_item = new QGraphicsSimpleTextItem{text};
  detail::addText(str_text_item, x, y, scene);
}

// constexpr int
constexpr int NUMBER_WIDTH = 50;
constexpr int COLUMN_WIDTH = NUMBER_WIDTH + 10;

inline void addText(int value, int x, int y, QGraphicsScene& scene) {
  /// this function is very expensive to call
  auto int_text_item = new QGraphicsSimpleTextItem{QString::number(value)};
  // alignment to the right
  x += COLUMN_WIDTH - int_text_item->boundingRect().width();
  detail::addText(int_text_item, x, y, scene);
}

template<typename SI>
static int extractProperty(const SI& info, ShapeProperty prop) {

  int value;

  if (prop == ShapeProperty::SIZE) {
    value = info.size;
  } else if (prop == ShapeProperty::COUNT) {
    value = info.get_count();
  } else if (prop == ShapeProperty::HEIGHT) {
    value = info.height;
  } else {
    abort();
    value = -1;
  }

  return value;
}

static void drawAnalysisHistogram(QGraphicsScene* scene, SimilarShapesWindow* ssw,
                           ShapeProperty prop, std::vector<SubtreeInfo>& vec) {
  if (vec.size() == 0) return;
  int curr_y = 40;
  int max_value = maxShapeValue<SubtreeInfo>(vec, prop);

  for (auto& shape : vec) {

    int value = extractProperty<SubtreeInfo>(shape, prop);

    const int rect_max_w = ShapeRect::SELECTION_WIDTH;
    const int rect_width = rect_max_w * value / max_value;
    auto rect = new ShapeRect(0, curr_y, rect_width, *shape.node, ssw);
    rect->addToScene(scene);

    if (shape.marked) {
      auto flag = new QGraphicsRectItem(0, curr_y - 5, 10, 10);
      flag->setBrush(Qt::yellow);
      scene->addItem(flag);
    }

    /// NOTE(maxim): drawing text is really expensive
    /// TODO(maxim): only draw if visible

    addText(shape.height, 10 + COLUMN_WIDTH * 0, curr_y, *scene);
    addText(shape.count,  10 + COLUMN_WIDTH * 1, curr_y, *scene);
    addText(shape.size,   10 + COLUMN_WIDTH * 2, curr_y, *scene);

    curr_y += ShapeRect::HEIGHT + 1;
  }
}

/// for comparison
static void drawAnalysisHistogram(QGraphicsScene* scene, SimilarShapesWindow* ssw,
                           ShapeProperty prop, std::vector<SubtreeInfo2>& vec) {

  if (vec.size() == 0) return;
  int curr_y = 40;
  int max_value = maxShapeValue<SubtreeInfo2>(vec, prop);

  for (auto& shape : vec) {

    int value = extractProperty<SubtreeInfo2>(shape, prop);

    const int rect_max_w = ShapeRect::SELECTION_WIDTH;
    const int rect_width = rect_max_w * value / max_value;
    auto rect = new ShapeRect(0, curr_y, rect_width, *shape.node, ssw);
    rect->addToScene(scene);

    /// NOTE(maxim): drawing text is really expensive
    /// TODO(maxim): only draw if visible

    addText(shape.height, 10 + COLUMN_WIDTH * 0, curr_y, *scene);
    addText(shape.count_ex1,  10 + COLUMN_WIDTH * 1, curr_y, *scene);
    addText(shape.count_ex2,  10 + COLUMN_WIDTH * 2, curr_y, *scene);
    addText(shape.size,   10 + COLUMN_WIDTH * 3, curr_y, *scene);

    curr_y += ShapeRect::HEIGHT + 1;
  }
}

void SimilarShapesWindow::drawAlternativeHistogram() {

  std::vector<SubtreeInfo> vec;
  vec.reserve(m_identicalGroups.size());

  auto& na = node_tree.getNA();
  VisualNode* root = node_tree.getRoot();

  root->unhideAll(na);
  root->layout(na);

  // qDebug() << "identical groups: " << m_identicalGroups.size();

  for (const auto& group : m_identicalGroups) {
    const int count = group.size();
    if (count == 0) continue;

    VisualNode* node = group[0];
    const int height = node->getShape()->depth();

    const int size = shapeSize(*node->getShape());

    if (!filters.check(height, count)) {
      continue;
    }

    vec.push_back({node, size, height, count, false});
  }

  sortSubtrees(vec, m_sortType);

  drawAnalysisHistogram(m_scene.get(), this, m_histType, vec);

}

void SimilarShapesWindow::drawComparisonHistogram() {

  auto& na = node_tree.getNA();
  VisualNode* root = node_tree.getRoot();

  root->unhideAll(na);
  root->layout(na);


  std::vector<SubtreeInfo2> vec;
  vec.reserve(m_identicalGroups.size());

  for (auto& group : m_identicalGroups) {

    const int total_count = group.size();
    if (total_count == 0) continue;

    int count_ex1 = 0;
    int count_ex2 = 0;

    for (auto* n : group) {
      if (node2ex_id->at(n) == 0) {
        ++count_ex1;
      } else {
        ++count_ex2;
      }
    }

    VisualNode* node = group[0];
    const int height = node->getShape()->depth();
    const int size = shapeSize(*node->getShape());

    if (!filters.check(height, total_count)) {
      continue;
    }

    vec.push_back({node, size, height, count_ex1, count_ex2});

  }

  drawAnalysisHistogram(m_scene.get(), this, m_histType, vec);

}

#ifdef MAXIM_DEBUG
/// copy is intentional
static void save_partition(vector<vector<VisualNode*>> vecs) {

  for (auto& v : vecs) {
    std::sort(begin(v), end(v),
      [](const VisualNode* lhs, const VisualNode* rhs) {
        return lhs->debug_id < rhs->debug_id;
    });
  }

  std::sort(begin(vecs), end(vecs),
    [](const vector<VisualNode*>& lhs, const vector<VisualNode*>& rhs) {

      if (lhs.size() == rhs.size()) {
        for (auto i = 0u; i < lhs.size(); ++i) {
          if (lhs[i]->debug_id == rhs[i]->debug_id) continue;
          return lhs[i]->debug_id < rhs[i]->debug_id;
        }
      } else {
        return lhs.size() < rhs.size();
      }
  });

  QString tmp_str;
  for (auto& v : vecs) {
    for (auto* n : v) {
      tmp_str += QString::number(n->debug_id) + " ";
    }

    tmp_str += "\n";
  }
  Utils::writeToFile(tmp_str);
}

#endif

void SimilarShapesWindow::updateHistogram() {

  m_scene.reset(new QGraphicsScene{});
  hist_view->setScene(m_scene.get());

  addText("hight", 10, 10, *m_scene);
  addText("count", 10 + COLUMN_WIDTH, 10, *m_scene);
  addText("size", 10 + COLUMN_WIDTH * 2, 10, *m_scene);

  switch (simType) {
    case SimilarityType::SHAPE:

      if (!shapes_cached) {
        updateShapesData();
        shapes_cached = true;
      }

      drawHistogram();
      break;
    case SimilarityType::SUBTREE: {

      if (!subtrees_cached) {

          /// TODO(maxim): get rid of the unnecessary copy here:
        perfHelper.begin("identical_shapes");
        m_identicalGroups = subtrees::findIdentical(node_tree);
        perfHelper.end();

          subtrees_cached = true;
          qDebug() << "identical groups:" << m_identicalGroups.size();

        #ifdef MAXIM_DEBUG
          auto str = "IdenticalGroups: " + std::to_string(m_identicalGroups.size());
          debug_label.setText(str.c_str());

          if (Settings::get_bool("save_shapes_to_file")) {
            save_partition(m_identicalGroups);
          }

        #endif

      }

      if (m_type == AnalysisType::SINGLE) {
        drawAlternativeHistogram();
      } else {

        drawComparisonHistogram();
      }
      break;
    }
  }
}

void SimilarShapesWindow::drawHistogram() {

  /// TODO(maxim): is it okay to make copies here?

  /// Apply filters
  std::vector<ShapeInfo> shapesShown;
  shapesShown.reserve(shapes.size());

  for (auto& shape : shapes) {
    if (!filters.check(shape.height, shape.nodes.size())) {
      continue;
    }
    shapesShown.push_back(shape);
  }

  /// TODO(maxim): I don't need to make copies here anymore
  std::vector<SubtreeInfo> vec;
  vec.reserve(shapesShown.size());

  for (auto shape : shapesShown) {
    const int size = shape.size;
    const int height = shape.height;
    const int count = shape.nodes.size();
    const bool equal = detail::areShapesIdentical(node_tree, shape.nodes);
    vec.push_back({shape.nodes[0], size, height, count, !equal});
  }

  sortSubtrees(vec, m_sortType);

  perfHelper.begin("actually drawing the histogram");
  drawAnalysisHistogram(m_scene.get(), this, m_histType, vec);
  perfHelper.end();
}

// ---------------------------------------


Filters::Filters() {}

bool Filters::check(int depth, int count) {
  if (depth < m_minDepth) return false;
  if (count < m_minCount) return false;
  return true;
}

void Filters::setMinDepth(int val) { m_minDepth = val; }

void Filters::setMinCount(int val) { m_minCount = val; }




}
}

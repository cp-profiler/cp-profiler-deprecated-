#include "similar_shapes.hh"

#include <algorithm>
#include <chrono>

#include <QLabel>
#include <QScrollBar>
#include <QPushButton>
#include <QPaintEvent>
#include <QGraphicsRectItem>

#include <QGraphicsView>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QSplitter>
#include <QSpinBox>

#include "visualnode.hh"
#include "libs/perf_helper.hh"
#include "execution.hh"
#include "globalhelper.hh"
#include "tree_utils.hh"
#include "identical_shapes.hh"

#include "shape_rect.hh"
#include "similar_shape_algorithm.hh"
#include "subsumed_subtrees.hpp"

#include "shape_aggregation.hh"

using std::vector;

namespace cpprofiler {
namespace analysis {

static ShapeProperty interpretShapeProperty(const QString& str) {
  if (str == "size") return ShapeProperty::SIZE;
  if (str == "count") return ShapeProperty::COUNT;
  if (str == "height") return ShapeProperty::HEIGHT;
  abort();
  return {};
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

/// Sort elements of `vec` in place based on `prop`
template<typename SI>
static void sortSubtrees(std::vector<SI>& vec, ShapeProperty prop) {
  if (prop == ShapeProperty::SIZE) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.size > s2.size;
              });
  } else if (prop == ShapeProperty::COUNT) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.get_count() > s2.get_count();
              });
  } else if (prop == ShapeProperty::HEIGHT) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.height > s2.height;
              });
  }
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

static
std::vector<SubtreeInfo> toSubtreeInfoVec(const NodeTree& nt,
                                        const std::vector<ShapeInfo>& shapes) {

  std::vector<SubtreeInfo> res;
  res.reserve(shapes.size());

  for (auto shape : shapes) {
    /// TODO(maxim): figure out why there is empty entries
    if (shape.nodes.size() == 0) continue;
    const int size = shape.size;
    const int height = shape.height;
    const int count = shape.nodes.size();
    const bool equal = detail::areShapesIdentical(nt, shape.nodes);
    res.push_back({shape.nodes, size, height, count, !equal});
  }

  return res;
}

SimilarShapesWindow::SimilarShapesWindow(Execution& ex)
    : HistogramWindow{ex} {

  initInterface();

  updateHistogram();
}

SimilarShapesWindow::~SimilarShapesWindow() = default;

void highlightAllSubtrees(NodeTree& nt, GroupsOfNodes_t& groups) {

  tree_utils::applyToEachNode(nt, [](VisualNode* n) {
    n->setHighlighted(false);
  });

  for (auto& group : groups) {
    for (auto node : group) {
      node->setHighlighted(true);
    }
  }

  nt.treeModified();
}

void highlightAllSubtrees(NodeTree& nt, std::vector<ShapeInfo>& shapes) {

  tree_utils::applyToEachNode(nt, [](VisualNode* n) {
    n->setHighlighted(false);
  });

  for (auto& si : shapes) {
    for (auto node : si.nodes) {
      node->setHighlighted(true);
    }
  }

  nt.treeModified();
}

void SimilarShapesWindow::initInterface() {
    settingsLayout->addWidget(new QLabel{"Similarity Type:"}, 0, Qt::AlignRight);

    auto typeChoice = new QComboBox();
    typeChoice->addItems({"shape", "subtree"});
    settingsLayout->addWidget(typeChoice);

    connect(typeChoice, &QComboBox::currentTextChanged, [this](const QString& str) {
      if (str == "shape") {
          simType = SimilarityType::SHAPE;
      } else if (str == "subtree") {
          simType = SimilarityType::SUBTREE;
      }
      updateHistogram();
    });

    auto subsumedOption = new QCheckBox{"Keep subsumed"};
    settingsLayout->addWidget(subsumedOption);

    connect(subsumedOption, &QCheckBox::stateChanged, [this](int state) {
      settings.keepSubsumed = (state == Qt::Checked);
      shapes_cached = false;
      subtrees_cached = false;
      updateHistogram();
    });

    settingsLayout->addStretch();

    settingsLayout->addWidget(new QLabel{"Labels:"}, 0, Qt::AlignRight);

    const QStringList label_options{"Ignore", "Vars only", "Full labels"};

    auto labels_comp = new QComboBox{};
    labels_comp->addItems(label_options);
    settingsLayout->addWidget(labels_comp);

    connect(labels_comp, &QComboBox::currentTextChanged, [this, label_options](const QString& str) {

      if (str == label_options[0]) {
        settings.label_opt = LabelOption::IGNORE;
      } else if (str == label_options[1]) {
        settings.label_opt = LabelOption::VARS;
      } else if (str == label_options[2]) {
        settings.label_opt = LabelOption::FULL;
      } else {
        std::cerr << "case not handeled\n";
        return;
      }

      subtrees_cached = false;
      updateHistogram();
    });

    auto hideNotHighlighted = new QCheckBox{"Hide not selected"};
    hideNotHighlighted->setCheckState(Qt::Checked);
    settingsLayout->addWidget(hideNotHighlighted);

    connect(hideNotHighlighted, &QCheckBox::stateChanged, [this](int state) {
      settings.hideNotHighlighted = (state == Qt::Checked);
    });

    auto depthFilterSB = new QSpinBox{this};
    depthFilterSB->setMinimum(1);
    depthFilterSB->setValue(2);

    connect(depthFilterSB, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this] (int val) {
      filters.minDepth = val;
      updateHistogram();
    });

    auto countFilterSB = new QSpinBox{this};
    countFilterSB->setMinimum(1);
    countFilterSB->setValue(2);

    connect(countFilterSB, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int val) {
      filters.minCount = val;
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

    auto histChoiceCB = new QComboBox();
    histChoiceCB->addItem("size");
    histChoiceCB->addItem("count");
    histChoiceCB->addItem("height");
    connect(histChoiceCB, &QComboBox::currentTextChanged,
            [this](const QString& str) {
              m_histType = interpretShapeProperty(str);
              updateHistogram();
            });

    filtersLayout->addWidget(new QLabel("min height"));
    filtersLayout->addWidget(depthFilterSB);

    filtersLayout->addWidget(new QLabel("min count"));
    filtersLayout->addWidget(countFilterSB);

    filtersLayout->addStretch();

    filtersLayout->addWidget(new QLabel{"sort by: "});
    filtersLayout->addWidget(sortChoiceCB);

    filtersLayout->addWidget(new QLabel{"histogram: "});
    filtersLayout->addWidget(histChoiceCB);


    auto highlightAll = new QPushButton{"Highlight All"};
    connect(highlightAll, &QPushButton::clicked, [this]() {

      if (simType == SimilarityType::SUBTREE)
        highlightAllSubtrees(node_tree, m_identicalGroups);
      else if (simType == SimilarityType::SHAPE)
        highlightAllSubtrees(node_tree, shapes);

    });
    miscLayout->addWidget(highlightAll);

    auto aggregateBtn = new QPushButton{"Aggregate"};
    connect(aggregateBtn, &QPushButton::clicked, [this]() {

      vector<std::string> labels;

      for (auto& group : groups_shown) {

        if (group.size() < 2) continue;

        auto vec_pair = getLabelDiff(execution, group[0], group[1]);

        for (auto& label : vec_pair.first) {
          labels.push_back(label);
        }

        for (auto& label : vec_pair.second) {
          labels.push_back(label);
        }

      }

      new ShapeAggregationWin(std::move(labels));
    });
    miscLayout->addWidget(aggregateBtn);

    miscLayout->addStretch();
}

void SimilarShapesWindow::drawAnalysisHistogram(ShapeProperty prop,
                                                std::vector<SubtreeInfo>& vec) {
  addText(*m_scene.get(), 0, 0, "hight");
  addText(*m_scene.get(), 1, 0, "count");
  addText(*m_scene.get(), 2, 0, "size");

  if (vec.size() == 0) return;
  int row = 1;
  int max_value = maxShapeValue<SubtreeInfo>(vec, prop);

  for (auto& shape : vec) {
    int value = extractProperty<SubtreeInfo>(shape, prop);

    const int rect_max_w = ShapeRect::SELECTION_WIDTH;
    const int rect_width = rect_max_w * value / max_value;

    auto rect =
        new ShapeRect(0, row * ROW_HEIGHT, rect_width, this);

    /// copy into unique_ptr since the original is about to be destroyed
    auto temp_si = new SubtreeInfo(shape.nodes, shape.size, shape.height,
                                   shape.count, shape.marked);
    rect_to_si[rect] = std::unique_ptr<SubtreeInfo>(temp_si);
    rect->addToScene(m_scene.get());

    if (shape.marked) {
      auto flag = new QGraphicsRectItem(0, row * ROW_HEIGHT + 3, 10, 10);
      flag->setBrush(Qt::yellow);
      m_scene->addItem(flag);
    }

    addText(*m_scene.get(), 0, row, shape.height);
    addText(*m_scene.get(), 1, row, shape.count);
    addText(*m_scene.get(), 2, row, shape.size);


    ++row;
  }
}

#ifdef MAXIM_DEBUG
/// copy is intended
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

      std::cerr << "should not reach here\n";
      return false;
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

static GroupsOfNodes_t shapesToGroups(const vector<ShapeInfo>& vec) {

  GroupsOfNodes_t result;
  result.reserve(vec.size());

  for (auto& si : vec) {
    result.push_back(si.nodes);
  }

  return result;
}

void SimilarShapesWindow::updateHistogram() {

  m_scene.reset(new QGraphicsScene{});
  hist_view->setScene(m_scene.get());

  switch (simType) {
    case SimilarityType::SHAPE:

      if (!shapes_cached) {
        
        perfHelper.begin("shapes: analyse");
        shapes = shapesToGroups(runSimilarShapes(node_tree));
        perfHelper.end();

        shapes_cached = true;

      }

      groups_shown = shapes;

    break;
    case SimilarityType::SUBTREE: {

      if (!subtrees_cached) {

          /// TODO(maxim): get rid of the unnecessary copy here:
        perfHelper.begin("identical_shapes");
        m_identicalGroups = subtrees::findIdentical(execution, settings.label_opt);
        perfHelper.end();
        subtrees_cached = true;
      }

      groups_shown = m_identicalGroups;

#ifdef MAXIM_DEBUG
      auto str = "IdenticalGroups: " + std::to_string(m_identicalGroups.size());
      debug_label.setText(str.c_str());

      if (Settings::get_bool("save_shapes_to_file")) {
        save_partition(m_identicalGroups);
      }
#endif

      break;
    }
  }

  groups_shown = filterOutUnique(groups_shown);

  if (!settings.keepSubsumed) {
    perfHelper.begin("subsumed shapes elimination");
    eliminateSubsumed(node_tree, groups_shown);
    perfHelper.end();
  }

  /// some shapes may have become unique
  groups_shown = filterOutUnique(groups_shown);

  drawHistogram();

}

void SimilarShapesWindow::drawHistogram() {

  std::vector<SubtreeInfo> vec;
  vec.reserve(m_identicalGroups.size());

  auto& na = node_tree.getNA();
  VisualNode* root = node_tree.getRoot();

  root->unhideAll(na);
  root->layout(na);

  for (const auto& group : groups_shown) {
    const int count = group.size();
    if (count == 0) continue;

    VisualNode* node = group[0];
    const int height = node->getShape()->depth();

    const int size = shapeSize(*node->getShape());

    if (!filters.check(height, count)) {
      continue;
    }

    vec.push_back({group, size, height, count, false});
  }

  sortSubtrees(vec, m_sortType);

  drawAnalysisHistogram(m_histType, vec);

}

// ---------------------------------------


}
}

#include "subtree_comp_win.hh"

#include "identical_shapes.hh"
#include "subtree_canvas.hh"
#include "nodetree.hh"

#include <QAbstractScrollArea>
#include <QGraphicsView>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QSpinBox>

namespace cpprofiler {
namespace analysis {

static ShapeProperty2 interpretShapeProperty(const QString& str) {
  if (str == "size") return ShapeProperty2::SIZE;
  if (str == "count") return ShapeProperty2::COUNT;
  if (str == "cc") return ShapeProperty2::CC;
  if (str == "height") return ShapeProperty2::HEIGHT;
  abort();
  return {};
}

struct SubtreeInfo2 {
    VisualNode* node; /// any node from the corresponding group
    int size;
    int height;
    int count_ex1;
    int count_ex2;

    int cc() const { return 10 * (float)count_ex1 * count_ex2 / (count_ex1 + count_ex1); }

    int get_count() const { return count_ex1 + count_ex2; }
};

/// Find a subtree from `vec` with the maximal `ShapeProperty` value and return that value
template<typename SI>
static int maxShapeValue(const std::vector<SI>& vec, ShapeProperty2 prop) {
  if (prop == ShapeProperty2::SIZE) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.size < s2.size;
        });
    return res.size;
  } else if (prop == ShapeProperty2::COUNT) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.get_count() < s2.get_count();
        });
    return res.get_count();
  } else if (prop == ShapeProperty2::CC) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.cc() < s2.cc();
        });
    return res.cc();
  } else if (prop == ShapeProperty2::HEIGHT) {
    const SI& res = *std::max_element(
        begin(vec), end(vec), [](const SI& s1, const SI& s2) {
          return s1.height < s2.height;
        });
    return res.height;
  }

  return 1;
}

template<typename SI>
static int extractProperty(const SI& info, ShapeProperty2 prop) {

  int value;

  if (prop == ShapeProperty2::SIZE) {
    value = info.size;
  } else if (prop == ShapeProperty2::COUNT) {
    value = info.get_count();
  } else if (prop == ShapeProperty2::CC) {
    value = info.cc();
  } else if (prop == ShapeProperty2::HEIGHT) {
    value = info.height;
  } else {
    abort();
    value = -1;
  }

  return value;
}

/// Sort elements of `vec` in place based on `prop`
template<typename SI>
static void sortSubtrees(std::vector<SI>& vec, ShapeProperty2 prop) {
  if (prop == ShapeProperty2::SIZE) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.size > s2.size;
              });
  } else if (prop == ShapeProperty2::COUNT) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.get_count() > s2.get_count();
              });
  } else if (prop == ShapeProperty2::CC) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.cc() > s2.cc();
              });
  } else if (prop == ShapeProperty2::HEIGHT) {
    std::sort(begin(vec), end(vec),
              [](const SI& s1, const SI& s2) {
                return s1.height > s2.height;
              });
  }
}


static void drawAnalysisHistogram(QGraphicsScene* scene, HistogramWindow* ssw,
                           ShapeProperty2 prop, std::vector<SubtreeInfo2>& vec) {

  addText(*scene, 0, 0, "hight");
  addText(*scene, 1, 0, "count_1");
  addText(*scene, 2, 0, "count_2");
  addText(*scene, 3, 0, "cc");
  addText(*scene, 4, 0, "size");

  if (vec.size() == 0) return;
  int row = 1;
  int max_value = maxShapeValue<SubtreeInfo2>(vec, prop);

  for (auto& shape : vec) {

    int value = extractProperty<SubtreeInfo2>(shape, prop);

    const int rect_max_w = ShapeRect::SELECTION_WIDTH;
    const int rect_width = rect_max_w * value / max_value;
    auto rect = new ShapeRect(0, row * ROW_HEIGHT, rect_width, *shape.node, ssw);
    rect->addToScene(scene);

    auto c1 = shape.count_ex1;
    auto c2 = shape.count_ex2;
    auto cc = (float)c1 * c2 / (c1 + c2);

    addText(*scene, 0, row, shape.height);
    addText(*scene, 1, row, c1);
    addText(*scene, 2, row, c2);
    addText(*scene, 3, row, cc);
    addText(*scene, 4, row, shape.size);

    ++row;
  }
}

SubtreeCompWindow::SubtreeCompWindow(NodeTree& nt, std::unique_ptr<ExecMap_t>&& map)
: HistogramWindow{nt}, node2ex_id{std::move(map)} {

    initInterface();
    updateHistogram();

}

SubtreeCompWindow::~SubtreeCompWindow() = default;

void SubtreeCompWindow::updateHistogram() {

    m_scene.reset(new QGraphicsScene{});
    hist_view->setScene(m_scene.get());

    if (!subtrees_cached) {
        m_identicalGroups = subtrees::findIdentical(node_tree);
        subtrees_cached = true;
    }

    drawComparisonHistogram();

}

void SubtreeCompWindow::drawComparisonHistogram() {

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

    if (!filters.check(height, std::max(count_ex1, count_ex2))) {
      continue;
    }

    vec.push_back({node, size, height, count_ex1, count_ex2});

  }

  sortSubtrees(vec, m_sortType);

  drawAnalysisHistogram(m_scene.get(), this, m_histType, vec);

}

void SubtreeCompWindow::initInterface() {

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
    sortChoiceCB->addItem("cc");
    sortChoiceCB->addItem("height");
    connect(sortChoiceCB, &QComboBox::currentTextChanged,
            [this](const QString& str) {
              m_sortType = interpretShapeProperty(str);
              updateHistogram();
            });

    auto histChoiceCB = new QComboBox();
    histChoiceCB->addItem("size");
    histChoiceCB->addItem("count");
    histChoiceCB->addItem("cc");
    histChoiceCB->addItem("height");
    connect(histChoiceCB, &QComboBox::currentTextChanged,
            [this](const QString& str) {
              m_histType = interpretShapeProperty(str);
              updateHistogram();
            });

    filtersLayout->addWidget(new QLabel("min height"));
    filtersLayout->addWidget(depthFilterSB);

    filtersLayout->addWidget(new QLabel("min count (both)"));
    filtersLayout->addWidget(countFilterSB);

    filtersLayout->addStretch();

    filtersLayout->addWidget(new QLabel{"sort by: "});
    filtersLayout->addWidget(sortChoiceCB);

    filtersLayout->addWidget(new QLabel{"histogram: "});
    filtersLayout->addWidget(histChoiceCB);

}












}}
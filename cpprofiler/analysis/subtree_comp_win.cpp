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

struct SubtreeInfo2 {
    VisualNode* node; /// any node from the corresponding group
    int size;
    int height;
    int count_ex1;
    int count_ex2;

    int get_count() const { return count_ex1 + count_ex2; }
};


static void drawAnalysisHistogram(QGraphicsScene* scene, HistogramWindow* ssw,
                           ShapeProperty prop, std::vector<SubtreeInfo2>& vec) {

  addText(*scene, 0, 0, "hight");
  addText(*scene, 1, 0, "count1");
  addText(*scene, 2, 0, "count2");
  addText(*scene, 3, 0, "size");

  if (vec.size() == 0) return;
  int row = 1;
  int max_value = maxShapeValue<SubtreeInfo2>(vec, prop);

  for (auto& shape : vec) {

    int value = extractProperty<SubtreeInfo2>(shape, prop);

    const int rect_max_w = ShapeRect::SELECTION_WIDTH;
    const int rect_width = rect_max_w * value / max_value;
    auto rect = new ShapeRect(0, row * ROW_HEIGHT, rect_width, *shape.node, ssw);
    rect->addToScene(scene);

    addText(*scene, 0, row, shape.height);
    addText(*scene, 1, row, shape.count_ex1);
    addText(*scene, 2, row, shape.count_ex2);
    addText(*scene, 3, row, shape.size);

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

    if (!filters.check(height, total_count)) {
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

}












}}
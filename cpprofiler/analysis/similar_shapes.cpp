#include "similar_shapes.hh"

#include <QLabel>
#include <QScrollBar>
#include <QPaintEvent>
#include "drawingcursor.hh"
#include "nodevisitor.hh"
#include "visualnode.hh"
#include <algorithm>
#include <chrono>
#include "libs/perf_helper.hh"
#include "nodetree.hh"

#include "identical_shapes.hpp"

namespace cpprofiler {
namespace analysis {

/// ******************************************
/// ************ SHAPE RECTANGLE *************
/// ******************************************

static const QColor transparent_red{255, 0, 0, 50};

class ShapeRect : public QGraphicsRectItem {

  VisualNode& m_node;
  SimilarShapesWindow* const m_ssWindow;

  void mousePressEvent(QGraphicsSceneMouseEvent*) override;

public:
  static constexpr int HEIGHT = 16;
  static constexpr int HALF_HEIGHT = HEIGHT / 2;
  static constexpr int PIXELS_PER_VALUE = 5;
  static constexpr int SELECTION_WIDTH = 600;

  ShapeRect(int x, int y, int width, VisualNode& n, SimilarShapesWindow*);

  void addToScene(QGraphicsScene* scene);
  QGraphicsRectItem visibleArea;

};

ShapeRect::ShapeRect(int x, int y, int width, VisualNode& node, SimilarShapesWindow* ssw)
    : QGraphicsRectItem(x, y - HALF_HEIGHT, SELECTION_WIDTH, HEIGHT, nullptr),
      visibleArea(x, y - HALF_HEIGHT + 1, width, HEIGHT - 2),
      m_node{node},
      m_ssWindow{ssw} {
  setBrush(Qt::white);
  QPen whitepen(Qt::white);
  setPen(whitepen);
  setFlag(QGraphicsItem::ItemIsSelectable);
  visibleArea.setBrush(transparent_red);
  visibleArea.setPen(whitepen);
}

void ShapeRect::addToScene(QGraphicsScene* scene) {
  scene->addItem(this);
  scene->addItem(&visibleArea);
}

void ShapeRect::mousePressEvent(QGraphicsSceneMouseEvent*) {
  m_ssWindow->highlightSubtrees(&m_node);
}

/// ******************************************

/// TODO(maxim): show all subtrees of a particular shape

static ShapeProperty interpretShapeProperty(const QString& str) {
  if (str == "size") return ShapeProperty::SIZE;
  if (str == "count") return ShapeProperty::COUNT;
  if (str == "height") return ShapeProperty::HEIGHT;
  abort();
  return {};
}

int Group::counter = 0;

std::ostream& operator<<(std::ostream& os, const VisualNode* n) {
  #ifdef MAXIM_DEBUG
      os << n->debug_id;
  #else
      os << (void*)n;
  #endif
  return os;
}

QDebug& operator<<(QDebug& os, const Group& g) {
  std::ostringstream oss;
  oss << "[ ";
  for (int i = 0; i < (int)g.items.size(); ++i) {
    if (i == g.splitter) {
      oss << "||";
    }
    auto& info = g.items[i];
    oss << "<" << info.alt << ", " << info.node << "> ";
  }
  if ((int)g.items.size() == g.splitter) {
      oss << "||";
  }
  oss << "]";
  os << oss.str().c_str();
  return os;
}

QDebug& operator<<(QDebug& os, const std::vector<Group>& groups) {
  for (auto i = 1u; i < groups.size(); ++i) {
    os << groups[i] << "\n";
  }
  return os;
}


int countShapes(std::multiset<ShapeI, CompareShapes>& mset) {
  int count = 0;

  for (auto it = mset.begin(), end = mset.end(); it != end; it = mset.upper_bound(*it)) {
    ++count;
  }

  return count;
}



/// conver to a vector of shape information
static std::vector<ShapeInfo> toShapeVector(std::multiset<ShapeI, CompareShapes>&& mset) {

  std::vector<ShapeInfo> shapes;
  shapes.reserve(mset.size() / 5); /// arbitrary

  auto it = mset.begin(); auto end = mset.end();

  while (it != end) {

    auto upper = mset.upper_bound(*it);

    shapes.push_back({it->sol, it->shape_size, it->shape_height, {it->node}, it->s});
    for (++it; it != upper; ++it) {
      shapes[shapes.size() - 1].nodes.push_back(it->node);
    }
  }

  return shapes;
}

/// NOTE: set's size is exactly n/2 (n = total nodes); why only half?
void eliminateSubsumed(std::multiset<ShapeI, CompareShapes>& mset) {

  // auto vec = toShapeVector(mset);


  /// 1. find all shapes of depth/height 1
  /// 2. find all shapes of depth/height 2
  /// 3. remove from set (1) all subsumed by any in (2)
}

SimilarShapesWindow::SimilarShapesWindow(TreeCanvas* tc, NodeTree& nt)
    : QDialog{tc}, m_tc{*tc}, node_tree{nt} {

  perfHelper.begin("shapes: analyse");
  shapes = toShapeVector(collectSimilarShapes());
  perfHelper.end();

  // perfHelper.begin("subsumed shapes elimination");
  // eliminateSubsumed(shapeSet);
  // perfHelper.end();

  initInterface();

  /// TODO(maxim): scroll area should be a part of the canvas
  m_ShapeCanvas.reset(new ShapeCanvas(m_scrollArea, node_tree));
  m_ShapeCanvas->show();

  updateHistogram();
}

int getNoOfSolvedLeaves(const NodeTree& node_tree, VisualNode* n) {
  int count = 0;

  CountSolvedCursor csc(n, node_tree.getNA(), count);
  PreorderNodeVisitor<CountSolvedCursor>(csc).run();

  return count;
}

void SimilarShapesWindow::highlightSubtrees(VisualNode* node) {

  /// TODO(maxim): does this do the right thing if SUBTREE?
  m_ShapeCanvas->showShape(node);

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
      m_tc.highlightSubtrees(shape_it->nodes);

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

      m_tc.highlightSubtrees(vec);
      break;
    }

  }

}

void SimilarShapesWindow::initInterface() {
  m_scene.reset(new QGraphicsScene{});

  hist_view = new QGraphicsView{this};
  hist_view->setScene(m_scene.get());
  hist_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  m_scrollArea = new QAbstractScrollArea{this};
  m_scrollArea->setAutoFillBackground(true);

  auto settingsLayout = new QHBoxLayout{};
  settingsLayout->addWidget(new QLabel{"Type:"});

  auto typeChoice = new QComboBox();
  typeChoice->addItem("shape");
  typeChoice->addItem("subtree");
  settingsLayout->addWidget(typeChoice);
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
  splitter->addWidget(m_scrollArea);
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

/// Compare nodes based on node type and number of children
static bool compareNodes(const VisualNode& n1, const VisualNode& n2) {
  if (n1.getStatus() != n2.getStatus()) {
    return false;
  }
  if (n1.getNumberOfChildren() != n2.getNumberOfChildren()) {
    return false;
  }

  return true;
}

/// Compare subtrees represented by root1 and root2
static bool compareSubtrees(const NodeTree& nt,
                            const VisualNode& root1,
                            const VisualNode& root2) {
  // compare roots
  bool equal = compareNodes(root1, root2);
  if (!equal) return false;

  // if nodes have children, compare them recursively:
  for (auto i = 0u; i < root1.getNumberOfChildren(); ++i) {

    auto new_root_1 = nt.getChild(root1, i);
    auto new_root_2 = nt.getChild(root2, i);

    bool equal = compareSubtrees(nt, *new_root_1, *new_root_2);
    if (!equal) return false;
  }

  return true;
}


static bool areShapesIdentical(const NodeTree& nt,
                               const std::vector<VisualNode*>& nodes) {
  auto first_node = nodes[0];

  if (nodes.size() == 1) return true;

  for (auto it = ++begin(nodes); it < end(nodes); ++it) {
    auto equal = compareSubtrees(nt, *first_node, **it);
    if (!equal) return false;
  }
  return true;
}

}

std::multiset<ShapeI, CompareShapes>
SimilarShapesWindow::collectSimilarShapes() {

  auto& na = node_tree.getNA();
  QHash<VisualNode*, int> nSols;

  auto getNoOfSolutions = [&na, &nSols] (VisualNode* n) -> int {
    int nSol = 0;
    switch (n->getStatus()) {
        case SOLVED:
          nSol = 1;
        break;
        case FAILED:
          nSol = 0;
        break;
        case SKIPPED:
          nSol = 0;
        case UNDETERMINED:
          nSol = 0;
        break;
        case BRANCH:
          nSol = 0;
          for (int i=n->getNumberOfChildren(); i--;) {
            nSol += nSols.value(n->getChild(na,i));
          }
        break;
        case STOP:
        case UNSTOP:
        case MERGING:
        break;    /// To avoid compiler warnings
    }

    return nSol;
  };

  std::multiset<ShapeI, CompareShapes> res;

  auto action = [&res, &nSols, getNoOfSolutions](VisualNode* n) {

    auto nSol = getNoOfSolutions(n);

    nSols[n] = nSol;
    if (n->getNumberOfChildren() > 0) {
      res.insert(cpprofiler::analysis::ShapeI(nSol,n));
    }
  };

  auto root = node_tree.getRoot();

  root->unhideAll(na);
  root->layout(na);

  m_tc.applyToEachNodePO(action);

  return res;
}

/// Find a subtree from `vec` with the maximal `ShapeProperty` value and return that value
static int maxShapeValue(const std::vector<SubtreeInfo>& vec, ShapeProperty prop) {
  if (prop == ShapeProperty::SIZE) {
    const SubtreeInfo& res = *std::max_element(
        begin(vec), end(vec), [](const SubtreeInfo& s1, const SubtreeInfo& s2) {
          return s1.size < s2.size;
        });
    return res.size;
  } else if (prop == ShapeProperty::COUNT) {
    const SubtreeInfo& res = *std::max_element(
        begin(vec), end(vec), [](const SubtreeInfo& s1, const SubtreeInfo& s2) {
          return s1.count < s2.count;
        });
    return res.count;
  } else if (prop == ShapeProperty::HEIGHT) {
    const SubtreeInfo& res = *std::max_element(
        begin(vec), end(vec), [](const SubtreeInfo& s1, const SubtreeInfo& s2) {
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

static int extractProperty(const SubtreeInfo& info, ShapeProperty prop) {

  int value;

  if (prop == ShapeProperty::SIZE) {
    value = info.size;
  } else if (prop == ShapeProperty::COUNT) {
    value = info.count;
  } else if (prop == ShapeProperty::HEIGHT) {
    value = info.height;
  } else {
    abort();
    value = -1;
  }

  return value;
}

void drawAnalysisHistogram(QGraphicsScene* scene, SimilarShapesWindow* ssw,
                           ShapeProperty prop, std::vector<SubtreeInfo>& vec) {
  int curr_y = 40;
  int max_value = maxShapeValue(vec, prop);

  for (auto& shape : vec) {

    int value = extractProperty(shape, prop);

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


/// TODO(maxim): make this more accurate
int shapeSize(const Shape& s) {
  int total_size = 0;

  int prev_l = 0;
  int prev_r = 0;

  for (auto i = 0u; i < s.depth(); ++i) {
    total_size += std::abs((s[i].r + prev_r) - (s[i].l + prev_l));
    prev_l += s[i].l;
    prev_r += s[i].r;
  }

  return total_size;
}

void SimilarShapesWindow::drawAlternativeHistogram() {

  std::vector<SubtreeInfo> vec;
  vec.reserve(m_identicalGroups.size());

  auto& na = node_tree.getNA();
  VisualNode* root = node_tree.getRoot();

  root->unhideAll(na);
  root->layout(na);

  for (const auto& group : m_identicalGroups) {
    const int count = group.size();
    if (count == 0) continue;

    VisualNode* node = group[0];
    const int height = node->getShape()->depth();

    const int size = shapeSize(*node->getShape());

    if (!filters.apply({height, count})) {
      continue;
    }

    vec.push_back({node, size, height, count, false});
  }

  sortSubtrees(vec, m_sortType);

  drawAnalysisHistogram(m_scene.get(), this, m_histType, vec);

}

void SimilarShapesWindow::updateHistogram() {

  m_scene.reset(new QGraphicsScene{});
  hist_view->setScene(m_scene.get());

  addText("hight", 10, 10, *m_scene);
  addText("count", 10 + COLUMN_WIDTH, 10, *m_scene);
  addText("size", 10 + COLUMN_WIDTH * 2, 10, *m_scene);

  switch (simType) {
    case SimilarityType::SHAPE:
      drawHistogram();
      break;
    case SimilarityType::SUBTREE: {

      if (m_identicalGroups.size() == 0) {

          /// TODO(maxim): get rid of the unnecessary copy here:
          m_identicalGroups = findIdenticalShapes(m_tc, node_tree);

        #ifdef MAXIM_DEBUG
          auto str = "IdenticalGroups: " + std::to_string(sizeof(m_identicalGroups));
          debug_label.setText(str.c_str());
        #endif

      }

      drawAlternativeHistogram();
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
    if (!filters.apply(shape)) {
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

namespace detail {

  Filters::Filters() {}

  bool Filters::apply(const ShapeInfo& si) {
    if (si.height < m_minDepth) return false;
    if (si.nodes.size() < m_minCount) return false;
    return true;
  }

  void Filters::setMinDepth(int val) { m_minDepth = val; }

  void Filters::setMinCount(int val) { m_minCount = val; }
}


/// ******************************************
/// ************* SHAPE CANVAS ***************
/// ******************************************

ShapeCanvas::ShapeCanvas(QAbstractScrollArea* sa, const NodeTree& nt)
    : QWidget{sa},
      m_ScrollArea{sa},
      m_NodeTree{nt} {}

void ShapeCanvas::paintEvent(QPaintEvent* event) {
  /// TODO(maxim): make a copy of a subtree to display here
  /// (so that it is never hidden)
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  if (!m_targetNode) return;

  const int view_w = m_ScrollArea->viewport()->width();
  const int view_h = m_ScrollArea->viewport()->height();

  int xoff = m_ScrollArea->horizontalScrollBar()->value();
  int yoff = m_ScrollArea->verticalScrollBar()->value();

  const BoundingBox bb = m_targetNode->getBoundingBox();

  const int w = bb.right - bb.left + Layout::extent;
  const int h =
      2 * Layout::extent + m_targetNode->getShape()->depth() * Layout::dist_y;

  // center the shape if small
  if (w < view_w) {
    xoff -= (view_w - w) / 2;
  }

  setFixedSize(view_w, view_h);

  int xtrans = -bb.left + (Layout::extent / 2);

  m_ScrollArea->horizontalScrollBar()->setRange(0, w - view_w);
  m_ScrollArea->verticalScrollBar()->setRange(0, h - view_h);
  m_ScrollArea->horizontalScrollBar()->setPageStep(view_w);
  m_ScrollArea->verticalScrollBar()->setPageStep(view_h);

  QRect origClip = event->rect();
  painter.translate(0, 30);
  painter.translate(xtrans - xoff, -yoff);
  QRect clip{origClip.x() - xtrans + xoff, origClip.y() + yoff,
             origClip.width(), origClip.height()};

  DrawingCursor dc{m_targetNode, m_NodeTree.getNA(), painter, clip, false};
  PreorderNodeVisitor<DrawingCursor>(dc).run();
}

void ShapeCanvas::showShape(VisualNode* node) {
  m_targetNode = node;

  shapeSize(*node->getShape());

  QWidget::update();
}

/// ******************************************

ShapeI::ShapeI(int sol0, VisualNode* node0)
    : sol(sol0), node(node0), s(Shape::copy(node->getShape())) {
  shape_size = shapeSize(*s);
  shape_height = s->depth();
}

ShapeI::~ShapeI() { Shape::deallocate(s); }

ShapeI::ShapeI(const ShapeI& sh)
    : sol(sh.sol),
      shape_size(sh.shape_size),
      shape_height(sh.shape_height),
      node(sh.node),
      s(Shape::copy(sh.s)) {}

ShapeI& ShapeI::operator=(const ShapeI& sh) {
  if (this != &sh) {
    Shape::deallocate(s);
    s = Shape::copy(sh.s);
    sol = sh.sol;
    shape_size = sh.shape_size;
    shape_height = sh.shape_height;
    node = sh.node;
  }
  return *this;
}

bool CompareShapes::operator()(const ShapeI& n1, const ShapeI& n2) const {
  if (n1.sol > n2.sol) return false;
  if (n1.sol < n2.sol) return true;

  const Shape& s1 = *n1.s;
  const Shape& s2 = *n2.s;

  if (n1.shape_height < n2.shape_height) return true;
  if (n1.shape_height > n2.shape_height) return false;

  for (int i = 0; i < n1.shape_height; ++i) {
    if (s1[i].l < s2[i].l) return false;
    if (s1[i].l > s2[i].l) return true;
    if (s1[i].r < s2[i].r) return true;
    if (s1[i].r > s2[i].r) return false;
  }
  return false;
}
}
}

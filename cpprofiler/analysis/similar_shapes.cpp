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

namespace cpprofiler {
namespace analysis {

/// TODO(maxim): show all subtrees of a particular shape
/// TODO(maxim): find 'exact' subtrees

class TreeStructure {
  // NodeAllocator m_na;

public:
  TreeStructure() {

  }

};

ShapeProperty interpretShapeProperty(const QString& str) {
  if (str == "size") return ShapeProperty::SIZE;
  if (str == "count") return ShapeProperty::COUNT;
  if (str == "height") return ShapeProperty::HEIGHT;
  abort();
  return {};
}

int Group::counter = 0;

// returns a pair <i,j> where i is a group index and j -- node's index within that group
std::pair<int, int> findNodeInGroups(
    const GroupsOfNodes_t& groups,
    std::unordered_map<const VisualNode*, PosInGroups>& node2groupID, const VisualNode* n) {
  // for (auto i = 1u; i < groups.size(); ++i) {
    auto g_idx = node2groupID[n].group_idx;
    auto idx = node2groupID[n].inner_idx;

    return std::make_pair(g_idx, idx);

    auto& group_items = groups[g_idx].items;
    for (auto j = 0u; j < group_items.size(); ++j) {
      auto& e = group_items[j];
      if (e.node == n) return std::make_pair(g_idx, j);
    }
  // }
  abort();
  return std::make_pair(-1, -1);
}

void separateNode(
    Group& g, std::unordered_map<const VisualNode*, PosInGroups>& node2groupID,
    int i) {
  /// swap the target element with the element pointed at by the splitter
  auto& el_1 = g.items[g.splitter];
  auto& el_2 = g.items[i];
  std::swap(el_1, el_2);
  std::swap(node2groupID[el_1.node].inner_idx,
            node2groupID[el_2.node].inner_idx);

  /// advance the splitter
  ++g.splitter;
}

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

QDebug& operator<<(QDebug& os, const GroupsOfNodes_t& groups) {
  for (int i = 1; i < (int)groups.size(); ++i) {
    os << groups[i] << "\n";
  }
  return os;
}

void splitGroups(GroupsOfNodes_t& groups, const std::vector<int>& g_to_split, std::unordered_map<const VisualNode*, PosInGroups>& node2groupID) {
  for (auto i = 0u; i < g_to_split.size(); ++i) {
    auto g_idx = g_to_split[i];
    auto& g = groups[g_idx];
    if (g.splitter == 0 || g.splitter == (int)g.items.size()) {
      g.splitter = 0;
      /// ----- don't need to split the group -----
      continue;
    }
    /// ----- need to split the group -----
    perfHelper.accumulate("actual splitting");
    auto it_splitter = begin(g.items) + g.splitter;
    
    Group new_group2{0, {it_splitter, end(g.items)}};

    g.items.erase(it_splitter, end(g.items));
    g.splitter = 0;

    /// ----- change group_id for nodes in the second group -----
    for (auto j = 0u; j < new_group2.items.size(); ++j) {
      auto* node = new_group2.items[j].node;
      node2groupID[node].group_idx = groups.size();
      node2groupID[node].inner_idx = j;
    }
    groups.push_back(std::move(new_group2));
    perfHelper.end("actual splitting");

    // ++i;
    // qDebug() << "after split groups: " << groups;

  }
}

int getSubtreeHeight(VisualNode* n, const NodeAllocator& na, GroupsOfNodes_t& groups) {
  int max = 0;

  int kids = n->getNumberOfChildren();

  if (kids == 0) {
    return 1;
  }

  for (int i = 0; i < kids; ++i) {
    auto kid = n->getChild(na, i);
    int h = getSubtreeHeight(kid, na, groups);
    auto& group_items = groups[h].items;
    group_items.push_back({i, kid});
    if (h > max) {
      max = h;
    }
  }

  return max + 1;
}

/// Groups nodes by height of their underlying subtree
GroupsOfNodes_t groupByHeight(const TreeCanvas& tc, const NodeTree& nt) {

  int max_depth = tc.get_stats().maxDepth;
  // int max_depth = 6;
  qDebug() << "max depth: " << max_depth;

  /// start from 1 for convenience
  GroupsOfNodes_t groups(max_depth + 1);

  auto& na = nt.getNA();
  auto* root = nt.getRootNode();

  getSubtreeHeight(root, na, groups);

  /// edge case of a root node
  groups[groups.size()-1].items.push_back({-1, root});

  return groups;
}


SimilarShapesWindow::SimilarShapesWindow(TreeCanvas* tc, const NodeTree& nt)
    : QDialog(tc), m_tc(tc), node_tree{nt}, shapeSet(CompareShapes{}), filters(*this) {
  perfHelper.begin("shapes: analyse");
  addNodesToMap();
  perfHelper.end();

  //--------------------------------------------
  //-------- FINDING IDENTICAL SUBTREES --------
  //--------------------------------------------

  auto& na = nt.getNA();

  /// ------ 0) group by height ------
  perfHelper.begin("shapes: group by height");
  GroupsOfNodes_t groups = groupByHeight(*tc, nt);
  perfHelper.end();


  /// ------ assign a group id to each node -------
  std::unordered_map<const VisualNode*, PosInGroups> node2groupID;
  for (auto group_idx = 1u; group_idx < groups.size(); ++group_idx) {
    auto& group = groups[group_idx];

    for (auto i = 0u; i < group.items.size(); ++i) {
      auto* node = group.items[i].node;
      node2groupID[node].group_idx = group_idx;
      node2groupID[node].inner_idx = i;
    }
  }

  /// ------ 1) linked list of pairs <begin, end>
  ///           for each group

  /// note: every node is identified by a pointer
  ///       `VisualNode*`

  /// note: every node has a link to its parent
  ///       through `n.getParent(na)`


  /// ------ 2) select the first block (with height 1)
  perfHelper.begin("shapes: minimisation algorithm");
  for (auto group_id = 1u; group_id < groups.size(); ++group_id) {

    auto block = groups[group_id];
    // qDebug() << "groups: " << groups;

    /// ------ 3) traverse 'left' elements of that block:
    /// ------ 
    ///             and separate it from the group

    std::vector<int> groups_to_split;

    // qDebug() << "left children:";
    for (auto& e: block.items) {
      if (e.alt == 0) {

        /// 3.1) get its parent
        auto parent = e.node->getParent(na);
        // std::cerr << parent->debug_id << " ";

        /// 3.2 )find it in groups
        perfHelper.accumulate("shapes: find node in groups");
        auto location = findNodeInGroups(groups, node2groupID, parent);
        perfHelper.end("shapes: find node in groups");
        // std::cerr << "in group: " << location.first << "\n";

        /// group g_idx will potentially need splitting
        auto g_idx = location.first;
        groups_to_split.push_back(g_idx);

        separateNode(groups[g_idx], node2groupID, location.second);
        // qDebug() << "groups: " << groups;
        /// split only affected groups
      }
    }
    // std::cerr << '\n';
    perfHelper.accumulate("shapes: split groups");
    splitGroups(groups, groups_to_split, node2groupID);
    perfHelper.end("shapes: split groups");
    groups_to_split.clear();
    // qDebug() << "groups: " << groups;

    // qDebug() << "right children:";
    for (auto& e: block.items) {
      if (e.alt == 1) {

        /// 3.1) get its parent
        auto parent = e.node->getParent(na);
        // std::cerr << parent->debug_id << " ";

        /// 3.2 )find it in groups
        perfHelper.accumulate("shapes: find node in groups");
        auto location = findNodeInGroups(groups, node2groupID, parent);
        perfHelper.end("shapes: find node in groups");

        auto g_idx = location.first;
        groups_to_split.push_back(g_idx);
        // std::cerr << "in group: " << location.first << "\n";
        separateNode(groups[g_idx], node2groupID, location.second);
        // qDebug() << "groups: " << groups;
        /// split only affected groups
      }
    }
    perfHelper.accumulate("shapes: split groups");
    splitGroups(groups, groups_to_split, node2groupID);
    perfHelper.end("shapes: split groups");
  }
  /// ----- sort the groups -----
#ifdef MAXIM_DEBUG
  sort(begin(groups), end(groups), [](const Group& lhs, const Group& rhs) {
    if (lhs.items.size() == 0 && rhs.items.size() == 0) {
      return false;
    }
    if (lhs.items.size() == 0) {
      return false;
    }
    if (rhs.items.size() == 0) {
      return true;
    }
    return lhs.items[0].node->debug_id < rhs.items[0].node->debug_id;
  });
#endif

  perfHelper.end();
  // qDebug() << "final groups: " << groups;
  perfHelper.total("shapes: find node in groups");
  perfHelper.total("shapes: split groups");
  perfHelper.total("actual splitting");

  //--------------------------------------------
  //--------------------------------------------

  scene.reset(new QGraphicsScene{});

  view = new QGraphicsView{this};
  view->setScene(scene.get());
  view->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  auto m_scrollArea = new QAbstractScrollArea{this};

  auto splitter = new QSplitter{this};

  splitter->addWidget(view);
  splitter->addWidget(m_scrollArea);
  splitter->setSizes(QList<int>{1, 1});  // for splitter to be centered

  auto depthFilterSB = new QSpinBox{this};
  depthFilterSB->setMinimum(1);
  depthFilterSB->setValue(2);
  QObject::connect(depthFilterSB, SIGNAL(valueChanged(int)), this,
                   SLOT(depthFilterChanged(int)));

  auto countFilterSB = new QSpinBox{this};
  countFilterSB->setMinimum(1);
  countFilterSB->setValue(2);
  QObject::connect(countFilterSB, SIGNAL(valueChanged(int)), this,
                   SLOT(countFilterChanged(int)));

  auto histChoiceCB = new QComboBox();
  histChoiceCB->addItem("size");
  histChoiceCB->addItem("count");
  histChoiceCB->addItem("height");
  connect(histChoiceCB, &QComboBox::currentTextChanged,
          [this](const QString& str) {
            m_histType = interpretShapeProperty(str);
            drawHistogram();
          });

  auto sortChoiceCB = new QComboBox();
  sortChoiceCB->addItem("size");
  sortChoiceCB->addItem("count");
  sortChoiceCB->addItem("height");
  connect(sortChoiceCB, &QComboBox::currentTextChanged,
          [this](const QString& str) {
            m_sortType = interpretShapeProperty(str);
            drawHistogram();
          });

  auto depthFilterLayout = new QHBoxLayout{};
  depthFilterLayout->addWidget(new QLabel("min height"));
  depthFilterLayout->addWidget(depthFilterSB);

  auto countFilterLayout = new QHBoxLayout{};
  countFilterLayout->addWidget(new QLabel("min count"));
  countFilterLayout->addWidget(countFilterSB);

  auto filtersLayout = new QHBoxLayout{};
  filtersLayout->addLayout(depthFilterLayout);
  filtersLayout->addLayout(countFilterLayout);
  filtersLayout->addStretch();
  filtersLayout->addWidget(new QLabel{"sort by: "});
  filtersLayout->addWidget(sortChoiceCB);
  filtersLayout->addWidget(new QLabel{"histogram: "});
  filtersLayout->addWidget(histChoiceCB);

  auto globalLayout = new QVBoxLayout{this};
  globalLayout->addWidget(splitter);
  globalLayout->addLayout(filtersLayout);

  // m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  // m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  m_scrollArea->setAutoFillBackground(true);

  perfHelper.begin("construct shapecanvas");
  /// TODO(maxim): get rid of the unnecessary copy here:
  m_identicalGroups = groups;
  shapeCanvas = new ShapeCanvas(m_scrollArea, tc, shapeSet, groups);
  perfHelper.end();
  shapeCanvas->show();

  // drawHistogram();
  drawAlternativeHistogram();
}

/// TODO(maxim): see if any of these could be reused in tree comparison
///              (or vice versa)
namespace detail {

/// equal if:
/// 1. same node type
/// 2. same number of children
inline bool compareNodes(const VisualNode& n1, const VisualNode& n2) {
  if (n1.getStatus() != n2.getStatus()) {
    return false;
  }
  if (n1.getNumberOfChildren() != n2.getNumberOfChildren()) {
    return false;
  }

  return true;
}

bool compareSubtrees(const NodeAllocator& na, const VisualNode& root1,
                     const VisualNode& root2) {
  // compare roots
  bool equal = compareNodes(root1, root2);
  if (!equal) return false;

  // if nodes have children, compare them recursively:
  for (uint i = 0; i < root1.getNumberOfChildren(); ++i) {
    auto new_root1 = root1.getChild(na, i);
    auto new_root2 = root2.getChild(na, i);
    bool equal = compareSubtrees(na, *new_root1, *new_root2);
    if (!equal) return false;
  }

  return true;
}

// compare subtrees of the same shape
bool areShapesIdentical(const NodeAllocator& na,
                        const std::multiset<ShapeI, CompareShapes>& set,
                        const ShapeI& shape) {
  auto first_it = set.lower_bound(shape);

  for (auto it = ++first_it; it != set.upper_bound(shape); ++it) {
    auto equal = compareSubtrees(na, *first_it->node, *it->node);
    if (!equal) return false;
  }
  return true;
}
}

void SimilarShapesWindow::addNodesToMap() {
  auto& na = m_tc->getExecution()->getNA();
  VisualNode* root = (na)[0];

  root->unhideAll(na);
  root->layout(na);
  SimilarShapesCursor ac(root, na, *this);
  PostorderNodeVisitor<SimilarShapesCursor>(ac).run();
}

int maxShapeValue(const std::vector<ShapeI>& shapes, ShapeProperty prop,
                  const std::multiset<ShapeI, CompareShapes>& set) {
  if (prop == ShapeProperty::SIZE) {
    const ShapeI& max_shape = *std::max_element(
        begin(shapes), end(shapes), [](const ShapeI& s1, const ShapeI& s2) {
          return s1.shape_size < s2.shape_size;
        });
    return max_shape.shape_size;
  } else if (prop == ShapeProperty::COUNT) {
    const ShapeI& max_shape = *std::max_element(
        begin(shapes), end(shapes), [&set](const ShapeI& s1, const ShapeI& s2) {
          return set.count(s1) < set.count(s2);
        });
    return set.count(max_shape);
  } else if (prop == ShapeProperty::HEIGHT) {
    const ShapeI& max_shape = *std::max_element(
        begin(shapes), end(shapes), [&set](const ShapeI& s1, const ShapeI& s2) {
          return s1.shape_height < s2.shape_height;
        });
    return max_shape.shape_height;
  }

  return 1;
}

void sortShapes(std::vector<ShapeI>& shapes, ShapeProperty prop,
  const std::multiset<ShapeI, CompareShapes>& set) {
  if (prop == ShapeProperty::SIZE) {
    std::sort(begin(shapes), end(shapes),
              [](const ShapeI& s1, const ShapeI& s2) {
                return s1.shape_size > s2.shape_size;
              });
  } else if (prop == ShapeProperty::COUNT) {
    std::sort(begin(shapes), end(shapes),
              [&set](const ShapeI& s1, const ShapeI& s2) {
                return set.count(s1) > set.count(s2);
              });
  } else if (prop == ShapeProperty::HEIGHT) {
    std::sort(begin(shapes), end(shapes),
              [](const ShapeI& s1, const ShapeI& s2) {
                return s1.shape_height > s2.shape_height;
              });
  }
}

namespace detail {

inline void addText(QGraphicsTextItem* text_item, int x, int y,
                    QGraphicsScene& scene) {
  // center the item vertically at y
  int text_y_offset = text_item->boundingRect().height() / 2;
  text_item->setPos(x, y - text_y_offset);
  scene.addItem(text_item);
}
};

inline void addText(const char* text, int x, int y, QGraphicsScene& scene) {
  /// will this memory be released?
  auto str_text_item = new QGraphicsTextItem{text};
  detail::addText(str_text_item, x, y, scene);
}

// constexpr int
constexpr int NUMBER_WIDTH = 50;
constexpr int COLUMN_WIDTH = NUMBER_WIDTH + 10;

inline void addText(int value, int x, int y, QGraphicsScene& scene) {
  auto int_text_item = new QGraphicsTextItem{QString::number(value)};
  // alignment to the right
  x += COLUMN_WIDTH - int_text_item->boundingRect().width();
  detail::addText(int_text_item, x, y, scene);
}

void SimilarShapesWindow::drawAlternativeHistogram() {
  scene.reset(new QGraphicsScene{});
  view->setScene(scene.get());

  int curr_y = 40;
  auto rect_max_w = ShapeRect::SELECTION_WIDTH;
  int max_value = 100;

  perfHelper.begin("displaying another histogram");
  for (auto it = m_identicalGroups.begin(), end = m_identicalGroups.end();
       it != end; ++it) {
    const Group& group = *it;

    int value = group.items.size();

    if (value == 0) continue;

    const int rect_width = rect_max_w * value / max_value;
    auto rect = new ShapeRect(0, curr_y, rect_width, group.items[0].node, shapeCanvas);
    rect->draw(scene.get());

    // auto sb_value = view->verticalScrollBar()->value();

    // rects_displayed++;
    curr_y += ShapeRect::HEIGHT + 1;
  }

  perfHelper.end();

}

void SimilarShapesWindow::drawHistogram() {
  qDebug() << "draw histogram\n";

  scene.reset(new QGraphicsScene{});
  view->setScene(scene.get());

  int curr_y = 40;
  int rects_displayed = 0;

  auto rect_max_w = ShapeRect::SELECTION_WIDTH;

  addText("hight", 10, 10, *scene);
  addText("count", 10 + COLUMN_WIDTH, 10, *scene);
  addText("size", 10 + COLUMN_WIDTH * 2, 10, *scene);

  perfHelper.begin("applying a filter");

  shapesShown.clear();
  shapesShown.reserve(shapeSet.size());

  for (auto it = shapeSet.begin(), end = shapeSet.end(); it != end;
       it = shapeSet.upper_bound(*it)) {
    if (!filters.apply(*it)) {
      continue;
    }

    shapesShown.push_back(*it);
  }

  perfHelper.end();

  perfHelper.begin("finding max");
  int max_value = maxShapeValue(shapesShown, m_histType, shapeSet);
  qDebug() << "max_value: " << max_value;
  perfHelper.end();

  perfHelper.begin("sorting a histogram");

  sortShapes(shapesShown, m_sortType, shapeSet);

  perfHelper.end();

  perfHelper.begin("displaying a histogram");
  for (auto it = shapesShown.begin(), end = shapesShown.end(); it != end;
       ++it) {
    const int shape_count = shapeSet.count(*it);
    const int shape_size = it->shape_size;
    const int shape_height = it->shape_height;

    auto equal = detail::areShapesIdentical(m_tc->getExecution()->getNA(), shapeSet, *it);

    int value;
    if (m_histType == ShapeProperty::SIZE) {
      value = shape_size;
    } else if (m_histType == ShapeProperty::COUNT) {
      value = shape_count;
    } else if (m_histType == ShapeProperty::HEIGHT) {
      value = shape_height;
    } else {
      abort();
      value = -1;
    }

    const int rect_width = rect_max_w * value / max_value;
    auto rect = new ShapeRect(0, curr_y, rect_width, it->node, shapeCanvas);
    rect->draw(scene.get());

    if (!equal) {
      auto flag = new QGraphicsRectItem(0, curr_y - 5, 10, 10);
      // flag.setPen(transparent_red);
      flag->setBrush(Qt::yellow);
      scene->addItem(flag);
    }

    /// NOTE(maxim): drawing text is really expensive
    /// TODO(maxim): only draw if visible

    // auto sb_value = view->verticalScrollBar()->value();

    addText(shape_height, 10 + COLUMN_WIDTH * 0, curr_y, *scene);
    addText(shape_count, 10 + COLUMN_WIDTH * 1, curr_y, *scene);
    addText(shape_size, 10 + COLUMN_WIDTH * 2, curr_y, *scene);

    rects_displayed++;
    curr_y += ShapeRect::HEIGHT + 1;
  }

  perfHelper.end();
}

void SimilarShapesWindow::depthFilterChanged(int val) {
  filters.setMinDepth(val);
  drawHistogram();
}

void SimilarShapesWindow::countFilterChanged(int val) {
  filters.setMinCount(val);
  drawHistogram();
}

// ---------------------------------------

Filters::Filters(const SimilarShapesWindow& ssw) : m_ssWindow(ssw) {}

bool Filters::apply(const ShapeI& si) {
  if (si.shape_height < m_minDepth) return false;
  if ((int)m_ssWindow.shapeSet.count(si) < m_minCount) return false;
  return true;
}

void Filters::setMinDepth(int val) { m_minDepth = val; }

void Filters::setMinCount(int val) { m_minCount = val; }

static const QColor transparent_red{255, 0, 0, 50};

ShapeRect::ShapeRect(int x, int y, int width, VisualNode* node, ShapeCanvas* sc,
                     QGraphicsItem* parent)
    : QGraphicsRectItem(x, y - HALF_HEIGHT, SELECTION_WIDTH, HEIGHT, parent),
      visibleArea(x, y - HALF_HEIGHT + 1, width, HEIGHT - 2),
      m_node{node},
      m_canvas{sc} {
  setBrush(Qt::white);
  QPen whitepen(Qt::white);
  setPen(whitepen);
  setFlag(QGraphicsItem::ItemIsSelectable);
  visibleArea.setBrush(transparent_red);
  visibleArea.setPen(whitepen);
}

void ShapeRect::draw(QGraphicsScene* scene) {
  scene->addItem(this);
  scene->addItem(&visibleArea);
}

VisualNode* ShapeRect::getNode() { return m_node; }

void ShapeRect::mousePressEvent(QGraphicsSceneMouseEvent*) {
  /// TODO(maxim): this should run in a parallel thread
  // perfHelper.begin("similar shapes: highlight on a tree");
  m_canvas->highlightShape(m_node);
  // perfHelper.end();
}

ShapeCanvas::ShapeCanvas(QAbstractScrollArea* sa, TreeCanvas* tc,
                         const std::multiset<ShapeI, CompareShapes>& set,
                         const std::vector<Group>& groups)
    : QWidget{sa},
      m_sa{sa},
      m_tc{tc},
      m_shapesSet{set},
      m_identicalGroups{groups} {}

void ShapeCanvas::paintEvent(QPaintEvent* event) {
  /// TODO(maxim): make a copy of a subtree to display here
  /// (so that it is never hidden)
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  auto it = m_shapesSet.begin();
  if (m_targetNode == nullptr) {
    m_targetNode = it->node;
  }

  int view_w = m_sa->viewport()->width();
  int view_h = m_sa->viewport()->height();

  int xoff = m_sa->horizontalScrollBar()->value();
  int yoff = m_sa->verticalScrollBar()->value();

  const BoundingBox bb = m_targetNode->getBoundingBox();

  int w = bb.right - bb.left + Layout::extent;
  int h =
      2 * Layout::extent + m_targetNode->getShape()->depth() * Layout::dist_y;

  // center the shape if small
  if (w < view_w) {
    xoff -= (view_w - w) / 2;
  }

  setFixedSize(view_w, view_h);

  xtrans = -bb.left + (Layout::extent / 2);

  m_sa->horizontalScrollBar()->setRange(0, w - view_w);
  m_sa->verticalScrollBar()->setRange(0, h - view_h);
  m_sa->horizontalScrollBar()->setPageStep(view_w);
  m_sa->verticalScrollBar()->setPageStep(view_h);

  QRect origClip = event->rect();
  painter.translate(0, 30);
  painter.translate(xtrans - xoff, -yoff);
  QRect clip{origClip.x() - xtrans + xoff, origClip.y() + yoff,
             origClip.width(), origClip.height()};

  DrawingCursor dc{m_targetNode, m_tc->getExecution()->getNA(), painter, clip, false};
  PreorderNodeVisitor<DrawingCursor>(dc).run();
}

void ShapeCanvas::highlightShape(VisualNode* node) {
  m_targetNode = node;
  /// NOTE(maxim): this will change layout -> shape depth!
  m_tc->highlightShape(node);
  QWidget::update();
}

void ShapeCanvas::scroll() { QWidget::update(); }

int shapeSize(const Shape& s) {
  int total_size = 0;

  int prev_l = 0;
  int prev_r = 0;

  for (int i = 0; i < s.depth(); ++i) {
    total_size += std::abs((s[i].r + prev_r) - (s[i].l + prev_l));
    prev_l = s[i].l;
    prev_r = s[i].r;
  }

  return total_size;
}

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

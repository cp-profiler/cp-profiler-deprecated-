#include "similar_shapes.hh"

#include <QLabel>
#include <QScrollBar>
#include <QPaintEvent>
#include "drawingcursor.hh"
#include "nodevisitor.hh"
#include "visualnode.hh"

namespace cpprofiler { namespace analysis {


SimilarShapesWindow::SimilarShapesWindow(TreeCanvas* tc)
 : QDialog(tc), m_tc(tc), shapesMap(CompareShapes{}),
  filters(*this)
{

  scene.reset(new QGraphicsScene{});

  view = new QGraphicsView{this};
  view->setScene(scene.get());
  view->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  auto m_scrollArea = new QAbstractScrollArea{this};

  auto splitter = new QSplitter{this};

  splitter->addWidget(view);
  splitter->addWidget(m_scrollArea);
  splitter->setSizes(QList<int>{1, 1}); // for splitter to be centered

  auto depthFilterSB = new QSpinBox{this};
  depthFilterSB->setMinimum(1);
  QObject::connect(depthFilterSB, SIGNAL(valueChanged(int)),
    this, SLOT(depthFilterChanged(int)));
  auto countFilterSB = new QSpinBox{this};
  countFilterSB->setMinimum(1);
  QObject::connect(countFilterSB, SIGNAL(valueChanged(int)),
    this, SLOT(countFilterChanged(int)));

  auto depthFilterLayout = new QHBoxLayout{};
  depthFilterLayout->addWidget(new QLabel("min depth"));
  depthFilterLayout->addWidget(depthFilterSB);

  auto countFilterLayout = new QHBoxLayout{};
  countFilterLayout->addWidget(new QLabel("min occurrence"));
  countFilterLayout->addWidget(countFilterSB);

  auto filtersLayout = new QHBoxLayout{};
  filtersLayout->addLayout(depthFilterLayout);
  filtersLayout->addLayout(countFilterLayout);
  filtersLayout->addStretch();

  auto globalLayout = new QVBoxLayout{this};
  globalLayout->addWidget(splitter);
  globalLayout->addLayout(filtersLayout);

  // m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  // m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  m_scrollArea->setAutoFillBackground(true);

  shapeCanvas = new ShapeCanvas(m_scrollArea, tc, shapesMap);
  shapeCanvas->show();
}

// constexpr int
constexpr int TEXT_WIDTH = 30;

void
SimilarShapesWindow::drawHistogram() {

  scene.reset(new QGraphicsScene{});
  view->setScene(scene.get());

  int curr_y = 0;
  int rects_displayed = 0;

  for(auto it = shapesMap.begin(), end = shapesMap.end();
           it != end; it = shapesMap.upper_bound(*it))
  {

    if (!filters.apply(*it)) { continue; }

    const int shapes_count = shapesMap.count(*it);
    auto rect = new ShapeRect(TEXT_WIDTH, curr_y, shapes_count,
                              it->node, shapeCanvas);
    rect->draw(scene.get());

    auto depth_str = QString::number((it->node)->getShape()->depth());
    auto text_item = new QGraphicsTextItem{depth_str};

    int text_y_offset = ShapeRect::HEIGHT / 2 - text_item->boundingRect().height() / 2;
    text_item->setPos(0, curr_y + text_y_offset);
    scene->addItem(text_item);

    rects_displayed++;
    curr_y += ShapeRect::HEIGHT + 1;

  }

  // TODO(maxim): set scroll bar height

  // view->verticalScrollBar()->setPageStep(ShapeRect::HEIGHT);
  // view->verticalScrollBar()->setRange(0, rects_displayed);



}

void
SimilarShapesWindow::depthFilterChanged(int val){
  filters.setMinDepth(val);
  drawHistogram();
}

void
SimilarShapesWindow::countFilterChanged(int val){
  filters.setMinCount(val);
  drawHistogram();
}


// ---------------------------------------

Filters::Filters(const SimilarShapesWindow& ssw)
 : m_ssWindow(ssw) {}

bool
Filters::apply(const ShapeI& si){
  if (si.s->depth() < m_minDepth) return false;
  if ((int)m_ssWindow.shapesMap.count(si) < m_minCount) return false;
  return true;
}

void
Filters::setMinDepth(int val){
  m_minDepth = val;
}

void
Filters::setMinCount(int val){
  m_minCount = val;
}

ShapeRect::ShapeRect(int x, int y, int value,
  VisualNode* node, ShapeCanvas* sc, QGraphicsItem * parent)
 : QGraphicsRectItem(x, y, SELECTION_WIDTH, HEIGHT, parent),
 visibleArea(x, y + 1, value * PIXELS_PER_VALUE, HEIGHT - 2),
 m_node{node}, m_canvas{sc}
 {
  setBrush(Qt::white);
  QPen whitepen(Qt::white);
  setPen(whitepen);
  setFlag(QGraphicsItem::ItemIsSelectable);
  visibleArea.setBrush(Qt::red);
  visibleArea.setPen(whitepen);
}

void
ShapeRect::draw(QGraphicsScene* scene){
  scene->addItem(this);
  scene->addItem(&visibleArea);
}

VisualNode*
ShapeRect::getNode(){
  return m_node;
}

void
ShapeRect::mousePressEvent (QGraphicsSceneMouseEvent* ) {
  m_canvas->highlightShape(m_node);
}

ShapeCanvas::ShapeCanvas(QAbstractScrollArea* sa, TreeCanvas* tc,
    const std::multiset<ShapeI, CompareShapes>& sm)
: QWidget{sa}, m_sa{sa}, m_tc{tc}, m_shapesMap{sm} {}

void
ShapeCanvas::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  auto it = m_shapesMap.begin();
  if (m_targetNode == nullptr) {
    m_targetNode = it->node;
  }

  int view_w = m_sa->viewport()->width();
  int view_h = m_sa->viewport()->height();

  int xoff = m_sa->horizontalScrollBar()->value();
  int yoff = m_sa->verticalScrollBar()->value();

  const BoundingBox bb = m_targetNode->getBoundingBox();

  int w = bb.right - bb.left + Layout::extent;
  int h = 2 * Layout::extent +
      m_targetNode->getShape()->depth() * Layout::dist_y;

  // center the shape if small
  if (w < view_w) { xoff -= (view_w - w) / 2; }

  setFixedSize(view_w, view_h);

  xtrans = -bb.left+(Layout::extent / 2);

  m_sa->horizontalScrollBar()->setRange(0, w - view_w);
  m_sa->verticalScrollBar()->setRange(0, h - view_h);
  m_sa->horizontalScrollBar()->setPageStep(view_w);
  m_sa->verticalScrollBar()->setPageStep(view_h);

  QRect origClip = event->rect();
  painter.translate(0, 30);
  painter.translate(xtrans - xoff, -yoff);
  QRect clip{origClip.x() - xtrans + xoff,
             origClip.y() + yoff,
             origClip.width(),
             origClip.height()};

  DrawingCursor dc{m_targetNode, *m_tc->get_na(), painter, clip, false};
  PreorderNodeVisitor<DrawingCursor>(dc).run();
}

void ShapeCanvas::highlightShape(VisualNode* node) {
  m_targetNode = node;
  m_tc->highlightShape(node);
  QWidget::update();
}

void
ShapeCanvas::scroll() {
  QWidget::update();
}


ShapeI::ShapeI(int sol0, VisualNode* node0)
  : sol(sol0), node(node0), s(Shape::copy(node->getShape())) {}

ShapeI::~ShapeI() { Shape::deallocate(s); }

ShapeI::ShapeI(const ShapeI& sh)
  : sol(sh.sol), node(sh.node), s(Shape::copy(sh.s)) {}

ShapeI& ShapeI::operator=(const ShapeI& sh) {
  if (this!=&sh) {
    Shape::deallocate(s);
    s = Shape::copy(sh.s);
    sol = sh.sol;
    node = sh.node;
  }
  return *this;
}

bool
CompareShapes::operator()(const ShapeI& n1, const ShapeI& n2) const {
  if (n1.sol > n2.sol) return false;
  if (n1.sol < n2.sol) return true;

  Shape* s1 = n1.s;
  Shape* s2 = n2.s;

  if (s1->depth() < s2->depth()) return true;
  if (s1->depth() > s2->depth()) return false;

  for (int i = 0; i < s1->depth(); i++) {
    if ((*s1)[i].l < (*s2)[i].l) return false;
    if ((*s1)[i].l > (*s2)[i].l) return true;
    if ((*s1)[i].r < (*s2)[i].r) return true;
    if ((*s1)[i].r > (*s2)[i].r) return false;
  }
  return false;
}

}}
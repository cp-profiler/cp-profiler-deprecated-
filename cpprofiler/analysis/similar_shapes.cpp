#include "similar_shapes.hh"

#include <QLabel>
#include <QScrollBar>
#include <QPaintEvent>
#include "drawingcursor.hh"
#include "nodevisitor.hh"
#include "visualnode.hh"

namespace cpprofiler { namespace analysis {


SimilarShapesWindow::SimilarShapesWindow(TreeCanvas* tc)
 : QDialog(tc), m_tc(tc), shapesMap(CompareShapes(*tc)),
  filters(*this)
{

  auto view = new QGraphicsView{this};
  view->setScene(&histScene);
  view->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  auto scrollArea = new QAbstractScrollArea{this};

  auto splitter = new QSplitter{this};

  splitter->addWidget(view);
  splitter->addWidget(scrollArea);
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

  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea->setAutoFillBackground(true);

  shapeCanvas = new ShapeCanvas(scrollArea, tc, shapesMap);
  shapeCanvas->show();
}

SimilarShapesWindow::~SimilarShapesWindow(void){
  delete shapeCanvas;
}

void
SimilarShapesWindow::drawHistogram(void) {
  QPen blackPen(Qt::black);

  histScene.clear();

  blackPen.setWidth(1);

  int y = 0, x = 0;

  for(auto it = shapesMap.begin(), end = shapesMap.end(); it != end; it = shapesMap.upper_bound(*it))
  {

    if (!filters.apply(*it)){
      continue;
    }

   // qDebug() << "shape: " << it->first << "node: " << it;

   // text = new QGraphicsTextItem;
   // text->setPos(-30, y - 5);
   // text->setPlainText(QString::number((it->node)->getShape()->depth()));
   //
   // histScene.addItem(text);

    int nShapes = shapesMap.count(*it);
    x = 5 * nShapes;

    auto rect = new ShapeRect(0, y, x, 20, it->node, this);
    rect->draw(&histScene);

    // text = new QGraphicsTextItem;
    // text->setPos(x + 10, y - 5);
    // text->setPlainText(QString::number(nShapes));
    //
    // histScene.addItem(text);
    y += 25;

  }

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
  if (m_ssWindow.shapesMap.count(si) < m_minCount) return false;
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

ShapeRect::ShapeRect(qreal x, qreal y, qreal width,
 qreal height, VisualNode* node, SimilarShapesWindow* ssw,
 QGraphicsItem * parent)
 :QGraphicsRectItem(x, y, 800, height, parent),
 selectionArea(x, y + 1, width, height - 2),
 _node(node), _ssWindow(ssw)
 {
  setBrush(Qt::white);
  QPen whitepen(Qt::white);
  setPen(whitepen);
  setFlag(QGraphicsItem::ItemIsSelectable);
  selectionArea.setBrush(Qt::red);
  selectionArea.setPen(whitepen);
}

void
ShapeRect::draw(QGraphicsScene* scene){
  scene->addItem(this);
  scene->addItem(&selectionArea);
}

VisualNode*
ShapeRect::getNode(void){
  return _node;
}

void
ShapeRect::mousePressEvent (QGraphicsSceneMouseEvent* ) {
  _ssWindow->shapeCanvas->highlightShape(_node);
}

ShapeCanvas::ShapeCanvas(QAbstractScrollArea* sa, TreeCanvas* tc,
    const std::multiset<ShapeI, CompareShapes>& sm)
: QWidget(sa), m_sa(sa), m_targetNode(nullptr), m_tc(tc), m_shapesMap{sm} {
}

void
ShapeCanvas::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  auto it = m_shapesMap.begin();
  if (m_targetNode == nullptr) {
    m_targetNode = it->node;
  }

  float scale = 1;
  int view_w = m_sa->viewport()->width();
  int view_h = m_sa->viewport()->height();

  int xoff = m_sa->horizontalScrollBar()->value()/scale;
  int yoff = m_sa->verticalScrollBar()->value()/scale;

  BoundingBox bb = m_targetNode->getBoundingBox();

  int w = static_cast<int>((bb.right - bb.left + Layout::extent) * scale);
  int h = static_cast<int>(2 * Layout::extent +
      m_targetNode->getShape()->depth() * Layout::dist_y * scale);

  // center the shape if small
  if (w < view_w) xoff -= (view_w - w)/2;

  setFixedSize(view_w, view_h) ;

  xtrans = -bb.left+(Layout::extent / 2);

  m_sa->horizontalScrollBar()->setRange(0, w - view_w);
  m_sa->verticalScrollBar()->setRange(0, h - view_h);
  m_sa->horizontalScrollBar()->setPageStep(view_w);
  m_sa->verticalScrollBar()->setPageStep(view_h);

  QRect origClip = event->rect();
  painter.translate(0, 30);
  // painter.scale(scale,scale);
  painter.translate(xtrans-xoff, -yoff);
  QRect clip(static_cast<int>(origClip.x()/scale-xtrans+xoff),
             static_cast<int>(origClip.y()/scale+yoff),
             static_cast<int>(origClip.width()/scale),
             static_cast<int>(origClip.height()/scale));

  DrawingCursor dc(m_targetNode, *m_tc->get_na(), painter, clip, false);
  PreorderNodeVisitor<DrawingCursor>(dc).run();
}

void ShapeCanvas::highlightShape(VisualNode* node) {
  m_targetNode = node;
  m_tc->highlightShape(node);
  QWidget::update();
}

void
ShapeCanvas::scroll(void) {
  QWidget::update();
}


ShapeI::ShapeI(int sol0, VisualNode* node0)
  : sol(sol0), node(node0), s(Shape::copy(node->getShape())) {}

ShapeI::~ShapeI(void) { Shape::deallocate(s); }

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

}}
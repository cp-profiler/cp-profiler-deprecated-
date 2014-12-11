#include <QtGui/QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QTimer>

#include <stack>
#include <fstream>
#include <exception>
#include <ctime>

#include "treecanvas.hh"
#include "treebuilder.hh"
#include "receiverthread.hh"

#include "data.hh"

#include "nodevisitor.hh"
#include "visualnode.hh"
#include "drawingcursor.hh"

int TreeCanvas::counter = 0;

TreeCanvas::TreeCanvas(QGridLayout* layout, receiverThread* receiver, CanvasType type, QWidget* parent)
    : QWidget(parent)
    , canvasType(type)
    , mutex(QMutex::Recursive)
    , layoutMutex(QMutex::Recursive)
    , finishedFlag(false)
    , compareNodes(false), compareNodesBeforeFP(false)
    , autoHideFailed(true), autoZoom(false)
    , refresh(500), refreshPause(0), smoothScrollAndZoom(false)
    , moveDuringSearch(false)
    , zoomTimeLine(500)
    , scrollTimeLine(1000), targetX(0), sourceX(0), targetY(0), sourceY(0)
    , targetW(0), targetH(0), targetScale(0)
    , layoutDoneTimerId(0)
    , shapesWindow(parent,  this)
    , shapesMap(CompareShapes(*this))
{
    QMutexLocker locker(&mutex);

    /// to distinguish between instances
    _id = TreeCanvas::counter++;

    _isUsed = false;

    ptr_receiver = receiver;
    _builder = new TreeBuilder(this);
    na = new Node::NodeAllocator(false);

    _data = new Data(this, na, false); // default data instance

    // timer disabled for now
    // should be removed
    timer = new QTimer(this);
    timer->start(2000);
    
    na->allocateRoot();

    root = (*na)[0];
    currentNode = root;
    root->setMarked(true);
    
    scale = LayoutConfig::defScale / 100.0;

    setAutoFillBackground(true);

    /// this one isn't really needed
    // connect(timer, SIGNAL(timeout(void)), ptr_receiver, SLOT(updateCanvas(void)));

    zoomPic.loadFromData(zoomToFitIcon, sizeof(zoomToFitIcon));

    autoZoomButton = new QToolButton();
    autoZoomButton->setCheckable(true);
    autoZoomButton->setIcon(zoomPic);

    layout->addWidget(autoZoomButton, 0,1, Qt::AlignHCenter);

    connect(autoZoomButton, SIGNAL(toggled(bool)), this, SLOT(setAutoZoom(bool)));


    connect(this, SIGNAL(autoZoomChanged(bool)), autoZoomButton, SLOT(setChecked(bool)));

    connect(_builder, SIGNAL(doneBuilding(void)), this, SLOT(finalizeCanvas(void)));

    connect(ptr_receiver, SIGNAL(update(int,int,int)), this,
            SLOT(layoutDone(int,int,int)));

    connect(ptr_receiver, SIGNAL(statusChanged(bool)), this,
            SLOT(statusChanged(bool)));

    connect(&scrollTimeLine, SIGNAL(frameChanged(int)),
            this, SLOT(scroll(int)));

    scrollTimeLine.setCurveShape(QTimeLine::EaseInOutCurve);

    scaleBar = new QSlider(Qt::Vertical, this);
    scaleBar->setObjectName("scaleBar");
    scaleBar->setMinimum(LayoutConfig::minScale);
    scaleBar->setMaximum(LayoutConfig::maxScale);
    scaleBar->setValue(LayoutConfig::defScale);
    connect(scaleBar, SIGNAL(valueChanged(int)),
            this, SLOT(scaleTree(int)));
    connect(this, SIGNAL(scaleChanged(int)), scaleBar, SLOT(setValue(int)));

    connect(&zoomTimeLine, SIGNAL(frameChanged(int)),
            scaleBar, SLOT(setValue(int)));
    zoomTimeLine.setCurveShape(QTimeLine::EaseInOutCurve);

    qRegisterMetaType<Statistics>("Statistics");

    qDebug() << "+++ new TreeCanvas created, id: " << _id;

    update();
}

TreeCanvas::~TreeCanvas(void) {
    if (root) {
        DisposeCursor dc(root,*na);
        PreorderNodeVisitor<DisposeCursor>(dc).run();
    }
    delete na;
}

///***********************
/// SIMILAR SUBTREES
///***********************
ShapeRect::ShapeRect(qreal x, qreal y, qreal width,
 qreal height, VisualNode* node, SimilarShapesWindow* ssw,
 QGraphicsItem * parent = 0)
 :QGraphicsRectItem(x, y, 800, height, parent),
 selectionArea(x, y + 1, width, height - 2),
 _node(node), _shapeCanvas(ssw->shapeCanvas), _ssWindow(ssw)
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
  _shapeCanvas->_targetNode = _node;
  _ssWindow->tc->highlightShape(_node);
  _shapeCanvas->QWidget::update();
}



SimilarShapesWindow::SimilarShapesWindow(QWidget* parent, TreeCanvas* tc) 
 : QDialog(parent), tc(tc), histScene(this), view(this),
  scrollArea(this), filters(tc)
{
  view.setScene(&histScene);

    applyLayouts();

  scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea.setAutoFillBackground(true);

  // for splitter to be centered
  QList<int> list = QList<int>() << 1 << 1;
  splitter.setSizes(list);

  depthFilterSB.setMinimum(1);
  countFilterSB.setMinimum(1);

  QObject::connect(&depthFilterSB, SIGNAL(valueChanged(int)),
    this, SLOT(depthFilterChanged(int)));
  QObject::connect(&countFilterSB, SIGNAL(valueChanged(int)),
    this, SLOT(countFilterChanged(int)));

  shapeCanvas = new ShapeCanvas(&scrollArea, tc);
  shapeCanvas->show();
}

SimilarShapesWindow::~SimilarShapesWindow(void){
  delete shapeCanvas;
}

void
SimilarShapesWindow::applyLayouts(void){
  setLayout(&globalLayout);
  splitter.addWidget(&view);
  splitter.addWidget(&scrollArea);
  view.setAlignment(Qt::AlignLeft | Qt::AlignTop);

  globalLayout.addLayout(&filtersLayout);

  filtersLayout.addLayout(&depthFilterLayout);
  filtersLayout.addLayout(&countFilterLayout);
  filtersLayout.addStretch();
  globalLayout.addWidget(&splitter);
    
  // depthFilterLayout.addWidget(&depthFilterLabel);
  depthFilterLayout.addWidget(new QLabel("min depth"));
  depthFilterLayout.addWidget(&depthFilterSB);
  countFilterLayout.addWidget(new QLabel("min occurrence"));
  countFilterLayout.addWidget(&countFilterSB);
}

void 
SimilarShapesWindow::drawHistogram(void) {
  QPen blackPen(Qt::black);
  ShapeRect * rect;

  histScene.clear();

  blackPen.setWidth(1);

  int y = 0, x = 0;

  for(std::multiset<ShapeI>::iterator it = tc->shapesMap.begin(),
   end = tc->shapesMap.end(); it != end; it = tc->shapesMap.upper_bound(*it))
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

    int nShapes = tc->shapesMap.count(*it);
    x = 5 * nShapes;

    rect = new ShapeRect(0, y, x, 20, it->node, this);
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
  filters.setMinDepth(static_cast<unsigned int>(val));
  drawHistogram();
}

void
SimilarShapesWindow::countFilterChanged(int val){
  filters.setMinCount(static_cast<unsigned int>(val));
  drawHistogram();
}

Filters::Filters(TreeCanvas* tc)
 : _minDepth(1), _minCount(1), _tc(tc) {}

bool
Filters::apply(const ShapeI& si){
  if (si.s->depth() < static_cast<int>(_minDepth)) return false;
  if (_tc->shapesMap.count(si) < _minCount) return false;
  return true;
}

void
Filters::setMinDepth(uint val){
  _minDepth = val;
}

void
Filters::setMinCount(uint val){
  _minCount = val;
}

ShapeCanvas::ShapeCanvas(QWidget* parent, TreeCanvas* tc) 
: QWidget(parent), _targetNode(NULL), _tc(tc) {
}

void
ShapeCanvas::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
    
  std::multiset<ShapeI>::iterator it;
  it = _tc->shapesMap.begin();
  if (_targetNode == NULL) {
    _targetNode = it->node;
  }
   
  QAbstractScrollArea* sa =
   static_cast<QAbstractScrollArea*>(parentWidget());
    
  float scale = 1;
  int view_w = sa->viewport()->width();
  int view_h = sa->viewport()->height();
    
  int xoff = sa->horizontalScrollBar()->value()/scale;
  int yoff = sa->verticalScrollBar()->value()/scale;
   
  BoundingBox bb = _targetNode->getBoundingBox();
  
  int w = static_cast<int>((bb.right - bb.left + Layout::extent) * scale);
  int h = static_cast<int>(2 * Layout::extent +
      _targetNode->getShape()->depth() * Layout::dist_y * scale);
  
  // center the shape if small
  if (w < view_w) xoff -= (view_w - w)/2;
   
  setFixedSize(view_w, view_h) ;
    
  xtrans = -bb.left+(Layout::extent / 2);
    
  sa->horizontalScrollBar()->setRange(0, w - view_w);
  sa->verticalScrollBar()->setRange(0, h - view_h);
  sa->horizontalScrollBar()->setPageStep(view_w);
  sa->verticalScrollBar()->setPageStep(view_h);
    
  QRect origClip = event->rect();
  painter.translate(0, 30);
  // painter.scale(scale,scale);
  painter.translate(xtrans-xoff, -yoff);
  QRect clip(static_cast<int>(origClip.x()/scale-xtrans+xoff),
             static_cast<int>(origClip.y()/scale+yoff),
             static_cast<int>(origClip.width()/scale),
             static_cast<int>(origClip.height()/scale));
   
  DrawingCursor dc(_targetNode, *_tc->na, painter, clip, false);
  PreorderNodeVisitor<DrawingCursor>(dc).run();
}

void
ShapeCanvas::scroll(void) {
  QWidget::update();
}

///***********************


//void
//TreeCanvas::addDoubleClickInspector(Inspector* i) {
//    doubleClickInspectors.append(QPair<Inspector*,bool>(i,false));
//}

//void
//TreeCanvas::activateDoubleClickInspector(int i, bool active) {
//    assert(i < doubleClickInspectors.size());
//    doubleClickInspectors[i].second = active;
//}

//void
//TreeCanvas::addSolutionInspector(Inspector* i) {
//    solutionInspectors.append(QPair<Inspector*,bool>(i,false));
//}

//void
//TreeCanvas::activateSolutionInspector(int i, bool active) {
//    assert(i < solutionInspectors.size());
//    solutionInspectors[i].second = active;
//}

//void
//TreeCanvas::addMoveInspector(Inspector* i) {
//    moveInspectors.append(QPair<Inspector*,bool>(i,false));
//}

//void
//TreeCanvas::activateMoveInspector(int i, bool active) {
//    assert(i < moveInspectors.size());
//    moveInspectors[i].second = active;
//}

//void
//TreeCanvas::addComparator(Comparator* c) {
//    comparators.append(QPair<Comparator*,bool>(c,false));
//}

//void
//TreeCanvas::activateComparator(int i, bool active) {
//    assert(i < comparators.size());
//    comparators[i].second = active;
//}

Data* TreeCanvas::getData(void) {
  return _data;
}

void
TreeCanvas::scaleTree(int scale0, int zoomx, int zoomy) {
    QMutexLocker locker(&layoutMutex);

    QSize viewport_size = size();
    QAbstractScrollArea* sa =
            static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());

    if (zoomx==-1)
        zoomx = viewport_size.width()/2;
    if (zoomy==-1)
        zoomy = viewport_size.height()/2;

    int xoff = (sa->horizontalScrollBar()->value()+zoomx)/scale;
    int yoff = (sa->verticalScrollBar()->value()+zoomy)/scale;

    BoundingBox bb;
    scale0 = std::min(std::max(scale0, LayoutConfig::minScale),
                      LayoutConfig::maxScale);
    scale = (static_cast<double>(scale0)) / 100.0;
    bb = root->getBoundingBox();
    int w =
            static_cast<int>((bb.right-bb.left+Layout::extent)*scale);
    int h =
            static_cast<int>(2*Layout::extent+
                             root->getShape()->depth()*Layout::dist_y*scale);

    sa->horizontalScrollBar()->setRange(0,w-viewport_size.width());
    sa->verticalScrollBar()->setRange(0,h-viewport_size.height());
    sa->horizontalScrollBar()->setPageStep(viewport_size.width());
    sa->verticalScrollBar()->setPageStep(viewport_size.height());
    sa->horizontalScrollBar()->setSingleStep(Layout::extent);
    sa->verticalScrollBar()->setSingleStep(Layout::extent);

    xoff *= scale;
    yoff *= scale;

    sa->horizontalScrollBar()->setValue(xoff-zoomx);
    sa->verticalScrollBar()->setValue(yoff-zoomy);

    emit scaleChanged(scale0);
    QWidget::update();
}

void
TreeCanvas::update(void) {
    QMutexLocker locker(&mutex);
    layoutMutex.lock();
    if (root != NULL) {
        root->layout(*na);
        BoundingBox bb = root->getBoundingBox();

        int w = static_cast<int>((bb.right-bb.left+Layout::extent)*scale);
        int h =
                static_cast<int>(2*Layout::extent+
                                 root->getShape()->depth()*Layout::dist_y*scale);
        xtrans = -bb.left+(Layout::extent / 2);

        QSize viewport_size = size();
        QAbstractScrollArea* sa =
                static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
        sa->horizontalScrollBar()->setRange(0,w-viewport_size.width());
        sa->verticalScrollBar()->setRange(0,h-viewport_size.height());
        sa->horizontalScrollBar()->setPageStep(viewport_size.width());
        sa->verticalScrollBar()->setPageStep(viewport_size.height());
        sa->horizontalScrollBar()->setSingleStep(Layout::extent);
        sa->verticalScrollBar()->setSingleStep(Layout::extent);
    }
    if (autoZoom)
        zoomToFit();
    layoutMutex.unlock();
    QWidget::update();
}

void
TreeCanvas::scroll(void) {
    QWidget::update();
}

void
TreeCanvas::layoutDone(int w, int h, int scale0) {
    targetW = w; targetH = h; targetScale = scale0;

    QSize viewport_size = size();
    QAbstractScrollArea* sa =
            static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
    sa->horizontalScrollBar()->setRange(0,w-viewport_size.width());
    sa->verticalScrollBar()->setRange(0,h-viewport_size.height());

    if (layoutDoneTimerId == 0)
        layoutDoneTimerId = startTimer(15);
}

void
TreeCanvas::statusChanged(bool finished) {
    if (finished) {
        update();
        centerCurrentNode();
    }
    emit statusChanged(currentNode, stats, finished);
}

int
TreeCanvas::getNoOfSolvedLeaves(VisualNode& n) {
  int count = 0;

  CountSolvedCursor csc(&n, na, count);
  PreorderNodeVisitor<CountSolvedCursor>(csc).run();

  return count;
}

CompareShapes::CompareShapes(TreeCanvas& tc) : _tc(tc) {}

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

void
TreeCanvas::analyzeSimilarSubtrees(void) {
  addNodesToMap();
  shapesWindow.drawHistogram();
  shapesWindow.show();  
}

void
TreeCanvas::addNodesToMap(void) {
  QMutexLocker locker_1(&mutex);
  QMutexLocker locker_2(&layoutMutex);
  shapesMap.clear();

  root->unhideAll(*na);
  root->layout(*na);
  AnalyzeCursor ac(root, *na, this);
  PostorderNodeVisitor<AnalyzeCursor>(ac).run();
}

void 
TreeCanvas::highlightShape(VisualNode* node) {
  QMutexLocker locker_1(&mutex);
  QMutexLocker locker_2(&layoutMutex);
  root->unhideAll(*na);
  root->layout(*na);
  UnhighlightCursor uhc(root, *na);
  PreorderNodeVisitor<UnhighlightCursor>(uhc).run();

  // highlight shape if it is not already hightlighted
  if (node != shapeHighlighted){

    shapeHighlighted = node;

    ShapeI toFind(getNoOfSolvedLeaves(node),node);

    // get all nodes with similar shape
    std::pair <std::multiset<ShapeI>::iterator,
    std::multiset<ShapeI>::iterator> range;
    range = shapesMap.equal_range(toFind);

    for (std::multiset<ShapeI>::iterator it = range.first; it!=range.second; ++it){
      VisualNode* targetNode = it->node;

      targetNode->setHighlighted(true);

      //HighlightCursor hc(targetNode, *na);
      //PreorderNodeVisitor<HighlightCursor>(hc).run();
    }
      
    HideNotHighlightedCursor hnhc(root,*na);
    PostorderNodeVisitor<HideNotHighlightedCursor>(hnhc).run();
      
  } else {
    shapeHighlighted = NULL;
  }
    
  update();
}

/// A stack item for depth first search
class SearchItem {
public:
    /// The node
    VisualNode* n;
    /// The currently explored child
    int i;
    /// The number of children
    int noOfChildren;
    /// Constructor
    SearchItem(VisualNode* n0, int noOfChildren0)
        : n(n0), i(-1), noOfChildren(noOfChildren0) {}
};

void
TreeCanvas::toggleHidden(void) {
    QMutexLocker locker(&mutex);
    currentNode->toggleHidden(*na);
    update();
    centerCurrentNode();
    emit statusChanged(currentNode, stats, true);
}

void
TreeCanvas::hideFailed(void) {
    QMutexLocker locker(&mutex);
    currentNode->hideFailed(*na);
    update();
    centerCurrentNode();
    emit statusChanged(currentNode, stats, true);
}

void
TreeCanvas::unhideAll(void) {
    QMutexLocker locker(&mutex);
    QMutexLocker layoutLocker(&layoutMutex);
    currentNode->unhideAll(*na);
    update();
    centerCurrentNode();
    emit statusChanged(currentNode, stats, true);
}

void
TreeCanvas::toggleStop(void) {
    QMutexLocker locker(&mutex);
    currentNode->toggleStop(*na);
    update();
    centerCurrentNode();
    emit statusChanged(currentNode, stats, true);
}

void
TreeCanvas::unstopAll(void) {
    QMutexLocker locker(&mutex);
    QMutexLocker layoutLocker(&layoutMutex);
    currentNode->unstopAll(*na);
    update();
    centerCurrentNode();
    emit statusChanged(currentNode, stats, true);
}

void
TreeCanvas::timerEvent(QTimerEvent* e) {
    if (e->timerId() == layoutDoneTimerId) {
        if (!smoothScrollAndZoom) {
            scaleTree(targetScale);
        } else {
            zoomTimeLine.stop();
            int zoomCurrent = static_cast<int>(scale*100);
            int targetZoom = targetScale;
            targetZoom = std::min(std::max(targetZoom, LayoutConfig::minScale),
                                  LayoutConfig::maxAutoZoomScale);
            zoomTimeLine.setFrameRange(zoomCurrent,targetZoom);
            zoomTimeLine.start();
        }
        QWidget::update();
        killTimer(layoutDoneTimerId);
        layoutDoneTimerId = 0;
    }
}

void
TreeCanvas::zoomToFit(void) {
    QMutexLocker locker(&layoutMutex);
    if (root != NULL) {
        BoundingBox bb;
        bb = root->getBoundingBox();
        QWidget* p = parentWidget();
        if (p) {
            double newXScale =
                    static_cast<double>(p->width()) / (bb.right - bb.left +
                                                       Layout::extent);
            double newYScale =
                    static_cast<double>(p->height()) / (root->getShape()->depth() *
                                                        Layout::dist_y +
                                                        2*Layout::extent);
            int scale0 = static_cast<int>(std::min(newXScale, newYScale)*100);
            if (scale0<LayoutConfig::minScale)
                scale0 = LayoutConfig::minScale;
            if (scale0>LayoutConfig::maxAutoZoomScale)
                scale0 = LayoutConfig::maxAutoZoomScale;

            if (!smoothScrollAndZoom) {
                scaleTree(scale0);
            } else {
                zoomTimeLine.stop();
                int zoomCurrent = static_cast<int>(scale*100);
                int targetZoom = scale0;
                targetZoom = std::min(std::max(targetZoom, LayoutConfig::minScale),
                                      LayoutConfig::maxAutoZoomScale);
                zoomTimeLine.setFrameRange(zoomCurrent,targetZoom);
                zoomTimeLine.start();
            }
        }
    }
}

void
TreeCanvas::centerCurrentNode(void) {
    QMutexLocker locker(&mutex);
    int x=0;
    int y=0;

    VisualNode* c = currentNode;
    while (c != NULL) {
        x += c->getOffset();
        y += Layout::dist_y;
        c = c->getParent(*na);
    }

    x = static_cast<int>((xtrans+x)*scale); y = static_cast<int>(y*scale);

    QAbstractScrollArea* sa =
            static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());

    x -= sa->viewport()->width() / 2;
    y -= sa->viewport()->height() / 2;

    sourceX = sa->horizontalScrollBar()->value();
    targetX = std::max(sa->horizontalScrollBar()->minimum(), x);
    targetX = std::min(sa->horizontalScrollBar()->maximum(),
                       targetX);
    sourceY = sa->verticalScrollBar()->value();
    targetY = std::max(sa->verticalScrollBar()->minimum(), y);
    targetY = std::min(sa->verticalScrollBar()->maximum(),
                       targetY);
    if (!smoothScrollAndZoom) {
        sa->horizontalScrollBar()->setValue(targetX);
        sa->verticalScrollBar()->setValue(targetY);
    } else {
        scrollTimeLine.stop();
        scrollTimeLine.setFrameRange(0,100);
        scrollTimeLine.setDuration(std::max(200,
                                            std::min(1000,
                                                     std::min(std::abs(sourceX-targetX),
                                                              std::abs(sourceY-targetY)))));
        scrollTimeLine.start();
    }
}

void
TreeCanvas::scroll(int i) {
    QAbstractScrollArea* sa =
            static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
    double p = static_cast<double>(i)/100.0;
    double xdiff = static_cast<double>(targetX-sourceX)*p;
    double ydiff = static_cast<double>(targetY-sourceY)*p;
    sa->horizontalScrollBar()->setValue(sourceX+static_cast<int>(xdiff));
    sa->verticalScrollBar()->setValue(sourceY+static_cast<int>(ydiff));
}


/// check what should be uncommented out.
void
TreeCanvas::inspectCurrentNode(bool, int) {
    QMutexLocker locker(&mutex);

    if (currentNode->isHidden()) {
        toggleHidden();
        return;
    }

    if (currentNode->getStatus() == MERGING) {
        toggleHidden();
        
        return;
    }

    int failedInspectorType = -1;
    int failedInspector = -1;
//    bool needCentering;

//    uint kids = currentNode->getNumberOfChildren();
    int depth = -1;
    for (VisualNode* p = currentNode; p != NULL; p=p->getParent(*na))
        depth++;
//    if (kids > 0) {
//        needCentering = true;
//        depth++;
//    }
    stats.maxDepth =
            std::max(stats.maxDepth, depth);

    try {
        switch (currentNode->getStatus()) {
            case UNDETERMINED:
            {
               // uint kids = currentNode->getNumberOfChildNodes(*na);
               // int depth = -1;
               // for (VisualNode* p = currentNode; p != NULL; p=p->getParent(*na))
               //     depth++;
               // if (kids > 0) {
               //     needCentering = true;
               //     depth++;
               // }
               // stats.maxDepth =
               //         std::max(stats.maxDepth, depth);
   //         if (currentNode->getStatus() == SOLVED) {
   //             emit solution(0);
   //         }
   //         emit statusChanged(currentNode,stats,true);
   //         // for (int i=0; i<moveInspectors.size(); i++) {
   //         //     if (moveInspectors[i].second) {
   //         //         failedInspectorType = 0;
   //         //         failedInspector = i;
   //         //         if (currentNode->getStatus() == FAILED) {
   //         //             if (!currentNode->isRoot()) {
   //         //                 Space* curSpace =
   //         //                         currentNode->getSpace(*na,curBest,c_d,a_d);
   //         //                 moveInspectors[i].first->inspect(*curSpace);
   //         //                 delete curSpace;
   //         //             }
   //         //         } else {
   //         //             moveInspectors[i].first->
   //         //                     inspect(*currentNode->getWorkingSpace());
   //         //         }
   //         //         failedInspectorType = -1;
   //         //     }
   //         // }
            }
            break;
            case FAILED:
            case STOP:
            case UNSTOP:
            case BRANCH:
            case SOLVED:
            case SKIPPED:
            case MERGING:
            break;
   //     {
   //         // SizeCursor sc(currentNode);
   //         // PreorderNodeVisitor<SizeCursor> pnv(sc);
   //         // int nodes = 1;
   //         // while (pnv.next()) { nodes++; }
   //         // std::cout << "sizeof(VisualNode): " << sizeof(VisualNode)
   //         //           << std::endl;
   //         // std::cout << "Size: " << (pnv.getCursor().s)/1024 << std::endl;
   //         // std::cout << "Nodes: " << nodes << std::endl;
   //         // std::cout << "Size / node: " << (pnv.getCursor().s)/nodes
   //         //           << std::endl;

   //         Space* curSpace;

   //         if (fix) {
   //             if (currentNode->isRoot() && currentNode->getStatus() == FAILED)
   //                 break;
   //             curSpace = currentNode->getSpace(*na,curBest,c_d,a_d);
   //             if (currentNode->getStatus() == SOLVED &&
   //                     curSpace->status() != SS_SOLVED) {
   //                 // in the presence of weakly monotonic propagators, we may have
   //                 // to use search to find the solution here
   //                 assert(curSpace->status() == SS_BRANCH &&
   //                        "Something went wrong - probably an incorrect brancher");
   //                 Space* dfsSpace = Gecode::dfs(curSpace);
   //                 delete curSpace;
   //                 curSpace = dfsSpace;
   //             }
   //         } else {
   //             if (currentNode->isRoot())
   //                 break;
   //             VisualNode* p = currentNode->getParent(*na);
   //             curSpace = p->getSpace(*na,curBest,c_d,a_d);
   //             switch (curSpace->status()) {
   //             case SS_SOLVED:
   //             case SS_FAILED:
   //                 break;
   //             case SS_BRANCH:
   //                 curSpace->commit(*p->getChoice(),
   //                                  currentNode->getAlternative(*na));
   //                 break;
   //             default:
   //                 GECODE_NEVER;
   //             }
   //         }

   //         if (inspectorNo==-1) {
   //             for (int i=0; i<doubleClickInspectors.size(); i++) {
   //                 if (doubleClickInspectors[i].second) {
   //                     failedInspectorType = 1;
   //                     failedInspector = i;
   //                     doubleClickInspectors[i].first->inspect(*curSpace);
   //                     failedInspectorType = -1;
   //                 }
   //             }
   //         } else {
   //             failedInspectorType = 1;
   //             failedInspector = inspectorNo;
   //             doubleClickInspectors[inspectorNo].first->inspect(*curSpace);
   //             failedInspectorType = -1;
   //         }
   //         delete curSpace;
   //     }
   //         break;
       }
    } catch (QException& e) {
       switch (failedInspectorType) {
       case 0:
           qFatal("Exception in move inspector %d: %s.\n Stopping.",
                  failedInspector, e.what());
           break;
       case 1:
           qFatal("Exception in double click inspector %d: %s.\n Stopping.",
                  failedInspector, e.what());
           break;
       default:
           qFatal("Exception: %s.\n Stopping.", e.what());
           break;
       }
   }

   currentNode->dirtyUp(*na);
   update();
   // if (needCentering)
   //     centerCurrentNode();
}

  int 
TreeCanvas::getNoOfSolvedLeaves(VisualNode* node) {
  int count;

  if (!node->hasSolvedChildren())
    return 0;
  return 1;
  CountSolvedCursor csc(node, *na, count);
  PreorderNodeVisitor<CountSolvedCursor>(csc).run();

  return count;
}


void
TreeCanvas::inspectBeforeFP(void) {
    inspectCurrentNode(false);
}

void
TreeCanvas::labelBranches(void) {
    QMutexLocker locker(&mutex);
    currentNode->labelBranches(*na, *this);
    update();
    centerCurrentNode();
    emit statusChanged(currentNode, stats, true);
}
void
TreeCanvas::labelPath(void) {
    QMutexLocker locker(&mutex);
    currentNode->labelPath(*na);
    update();
    centerCurrentNode();
    emit statusChanged(currentNode, stats, true);
}


void
TreeCanvas::stopSearch(void) {
    stopSearchFlag = true;
    layoutDoneTimerId = startTimer(15);
}

void
TreeCanvas::reset(bool isRestarts) {
    QMutexLocker locker(&mutex);

    qDebug() << "tc #" << _id << "is resetting";

    if (na) delete na;
    if (_data) delete _data;

    na = new Node::NodeAllocator(false);

    int rootIdx = na->allocateRoot();
    assert(rootIdx == 0); (void) rootIdx;
    root = (*na)[0];
    root->setMarked(true);
    currentNode = root;
    pathHead = root;
    scale = 1.0;
    stats = Statistics();
    for (int i=bookmarks.size(); i--;)
        emit removedBookmark(i);
    bookmarks.clear();
    // root->layout(*na);

    emit statusChanged(currentNode, stats, true);

    _data = new Data(this, na, isRestarts);

    _builder->reset(_data, na);

    _isUsed = false;

    update();
}

void
TreeCanvas::toggleSecondCanvas(void) {
  qDebug() << "comparing Trees";
}


void
TreeCanvas::bookmarkNode(void) {
    QMutexLocker locker(&mutex);
    if (!currentNode->isBookmarked()) {
        bool ok;
        QString text =
                QInputDialog::getText(this, "Add bookmark", "Name:",
                                      QLineEdit::Normal,"",&ok);
        if (ok) {
            currentNode->setBookmarked(true);
            bookmarks.append(currentNode);
            if (text == "")
                text = QString("Node ")+QString().setNum(bookmarks.size());
            emit addedBookmark(text);
        }
    } else {
        currentNode->setBookmarked(false);
        int idx = bookmarks.indexOf(currentNode);
        bookmarks.remove(idx);
        emit removedBookmark(idx);
    }
    currentNode->dirtyUp(*na);
    update();
}

void
TreeCanvas::setPath(void) {
    QMutexLocker locker(&mutex);
    if(currentNode == pathHead)
        return;

    pathHead->unPathUp(*na);
    pathHead = currentNode;

    currentNode->pathUp(*na);
    currentNode->dirtyUp(*na);
    update();
}

void
TreeCanvas::inspectPath(void) {
    QMutexLocker locker(&mutex);
    setCurrentNode(root);
    if (currentNode->isOnPath()) {
        inspectCurrentNode();
        int nextAlt = currentNode->getPathAlternative(*na);
        while (nextAlt >= 0) {
            setCurrentNode(currentNode->getChild(*na,nextAlt));
            inspectCurrentNode();
            nextAlt = currentNode->getPathAlternative(*na);
        }
    }
    update();
}

void
TreeCanvas::startCompareNodes(void) {
    QMutexLocker locker(&mutex);
    compareNodes = true;
    compareNodesBeforeFP = false;
    setCursor(QCursor(Qt::CrossCursor));
}

void
TreeCanvas::startCompareNodesBeforeFP(void) {
    QMutexLocker locker(&mutex);
    compareNodes = true;
    compareNodesBeforeFP = true;
    setCursor(QCursor(Qt::CrossCursor));
}

void
TreeCanvas::emitStatusChanged(void) {
    emit statusChanged(currentNode, stats, true);
}

void
TreeCanvas::navUp(void) {
    QMutexLocker locker(&mutex);

    VisualNode* p = currentNode->getParent(*na);

    setCurrentNode(p);

    if (p != NULL) {
        centerCurrentNode();
    }
}

void
TreeCanvas::navDown(void) {
    QMutexLocker locker(&mutex);
    if (!currentNode->isHidden()) {
        switch (currentNode->getStatus()) {
        case STOP:
        case UNSTOP:
        case MERGING:
        case BRANCH:
        {
            int alt = std::max(0, currentNode->getPathAlternative(*na));
            VisualNode* n = currentNode->getChild(*na,alt);
            setCurrentNode(n);
            centerCurrentNode();
            break;
        }
        case SOLVED:
        case FAILED:
        case SKIPPED:
        case UNDETERMINED:
            break;
        }
    }
}

void
TreeCanvas::navLeft(void) {
    QMutexLocker locker(&mutex);
    VisualNode* p = currentNode->getParent(*na);
    if (p != NULL) {
        int alt = currentNode->getAlternative(*na);
        if (alt > 0) {
            VisualNode* n = p->getChild(*na,alt-1);
            setCurrentNode(n);
            centerCurrentNode();
        }
    }
}

void
TreeCanvas::navRight(void) {
    QMutexLocker locker(&mutex);
    VisualNode* p = currentNode->getParent(*na);
    if (p != NULL) {
        uint alt = currentNode->getAlternative(*na);
        if (alt + 1 < p->getNumberOfChildren()) {
            VisualNode* n = p->getChild(*na,alt+1);
            setCurrentNode(n);
            centerCurrentNode();
        }
    }
}

void
TreeCanvas::navRoot(void) {
    QMutexLocker locker(&mutex);
    setCurrentNode(root);
    centerCurrentNode();
}

void
TreeCanvas::navNextSol(bool back) {
    QMutexLocker locker(&mutex);
    NextSolCursor nsc(currentNode,back,*na);
    PreorderNodeVisitor<NextSolCursor> nsv(nsc);
    nsv.run();
    VisualNode* n = nsv.getCursor().node();
    if (n != root) {
        setCurrentNode(n);
        centerCurrentNode();
    }
}

void
TreeCanvas::navPrevSol(void) {
    navNextSol(true);
}

void
TreeCanvas::exportNodePDF(VisualNode* n) {
#if QT_VERSION >= 0x040400
    QString filename = QFileDialog::getSaveFileName(this, tr("Export tree as pdf"), "", tr("PDF (*.pdf)"));
    if (filename != "") {
        QPrinter printer(QPrinter::ScreenResolution);
        QMutexLocker locker(&mutex);

        BoundingBox bb = n->getBoundingBox();
        printer.setFullPage(true);
        printer.setPaperSize(QSizeF(bb.right-bb.left+Layout::extent,
                                    n->getShape()->depth() * Layout::dist_y +
                                    Layout::extent), QPrinter::Point);
        printer.setOutputFileName(filename);
        QPainter painter(&printer);

        painter.setRenderHint(QPainter::Antialiasing);

        QRect pageRect = printer.pageRect();
        double newXScale =
                static_cast<double>(pageRect.width()) / (bb.right - bb.left +
                                                         Layout::extent);
        double newYScale =
                static_cast<double>(pageRect.height()) /
                (n->getShape()->depth() * Layout::dist_y +
                 Layout::extent);
        double printScale = std::min(newXScale, newYScale);
        painter.scale(printScale,printScale);

        int printxtrans = -bb.left+(Layout::extent / 2);

        painter.translate(printxtrans, Layout::dist_y / 2);
        QRect clip(0,0,0,0);
        DrawingCursor dc(n, *na, painter, clip);
        currentNode->setMarked(false);
        PreorderNodeVisitor<DrawingCursor>(dc).run();
        currentNode->setMarked(true);
    }
#else
    (void) n;
#endif
}

void
TreeCanvas::exportWholeTreePDF(void) {
#if QT_VERSION >= 0x040400
    exportNodePDF(root);
#endif
}

void
TreeCanvas::exportPDF(void) {
#if QT_VERSION >= 0x040400
    exportNodePDF(currentNode);
#endif
}

void
TreeCanvas::print(void) {
    QPrinter printer;
    if (QPrintDialog(&printer, this).exec() == QDialog::Accepted) {
        QMutexLocker locker(&mutex);

        BoundingBox bb = root->getBoundingBox();
        QRect pageRect = printer.pageRect();
        double newXScale =
                static_cast<double>(pageRect.width()) / (bb.right - bb.left +
                                                         Layout::extent);
        double newYScale =
                static_cast<double>(pageRect.height()) /
                (root->getShape()->depth() * Layout::dist_y +
                 2*Layout::extent);
        double printScale = std::min(newXScale, newYScale)*100;
        if (printScale<1.0)
            printScale = 1.0;
        if (printScale > 400.0)
            printScale = 400.0;
        printScale = printScale / 100.0;

        QPainter painter(&printer);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(printScale,printScale);
        painter.translate(xtrans, 0);
        QRect clip(0,0,0,0);
        DrawingCursor dc(root, *na, painter, clip);
        PreorderNodeVisitor<DrawingCursor>(dc).run();
    }
}

VisualNode*
TreeCanvas::eventNode(QEvent* event) {
    int x = 0;
    int y = 0;
    switch (event->type()) {
    case QEvent::ToolTip:
    {
        QHelpEvent* he = static_cast<QHelpEvent*>(event);
        x = he->x();
        y = he->y();
        break;
    }
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        x = me->x();
        y = me->y();
        break;
    }
    case QEvent::ContextMenu:
    {
        QContextMenuEvent* ce = static_cast<QContextMenuEvent*>(event);
        x = ce->x();
        y = ce->y();
        break;
    }
    default:
        return NULL;
    }
    QAbstractScrollArea* sa =
            static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
    int xoff = sa->horizontalScrollBar()->value()/scale;
    int yoff = sa->verticalScrollBar()->value()/scale;

    BoundingBox bb = root->getBoundingBox();
    int w =
            static_cast<int>((bb.right-bb.left+Layout::extent)*scale);
    if (w < sa->viewport()->width())
        xoff -= (sa->viewport()->width()-w)/2;
    
    VisualNode* n;
    n = root->findNode(*na,
                       static_cast<int>(x/scale-xtrans+xoff),
                       static_cast<int>((y-30)/scale+yoff));
    return n;
}

bool
TreeCanvas::event(QEvent* event) {
    if (mutex.tryLock()) {
        if (event->type() == QEvent::ToolTip) {
            VisualNode* n = eventNode(event);
            if (n != NULL) {
                QHelpEvent* he = static_cast<QHelpEvent*>(event);
                QToolTip::showText(he->globalPos(),
                                   QString(n->toolTip(*na).c_str()));
            } else {
                QToolTip::hideText();
            }
        }
        mutex.unlock();
    }
    return QWidget::event(event);
}

void
TreeCanvas::resizeToOuter(void) {
    if (autoZoom)
        zoomToFit();
}

void
TreeCanvas::paintEvent(QPaintEvent* event) {
    QMutexLocker locker(&layoutMutex);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QAbstractScrollArea* sa =
            static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
    int xoff = sa->horizontalScrollBar()->value()/scale;
    int yoff = sa->verticalScrollBar()->value()/scale;

    BoundingBox bb = root->getBoundingBox();
    int w =
            static_cast<int>((bb.right-bb.left+Layout::extent)*scale);
    if (w < sa->viewport()->width())
        xoff -= (sa->viewport()->width()-w)/2;

    QRect origClip = event->rect();
    painter.translate(0, 30);
    painter.scale(scale,scale);
    painter.translate(xtrans-xoff, -yoff);
    QRect clip(static_cast<int>(origClip.x()/scale-xtrans+xoff),
               static_cast<int>(origClip.y()/scale+yoff),
               static_cast<int>(origClip.width()/scale),
               static_cast<int>(origClip.height()/scale));
    DrawingCursor dc(root, *na, painter, clip);
    PreorderNodeVisitor<DrawingCursor>(dc).run();

    // int nodesLayouted = 1;
    // clock_t t0 = clock();
    // while (v.next()) { nodesLayouted++; }
    // double t = (static_cast<double>(clock()-t0) / CLOCKS_PER_SEC) * 1000.0;
    // double nps = static_cast<double>(nodesLayouted) /
    //   (static_cast<double>(clock()-t0) / CLOCKS_PER_SEC);
    // std::cout << "Drawing done. " << nodesLayouted << " nodes in "
    //   << t << " ms. " << nps << " nodes/s." << std::endl;

}

void
TreeCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
    if (mutex.tryLock()) {
        if(event->button() == Qt::LeftButton) {
            VisualNode* n = eventNode(event);
            if(n == currentNode) {
                inspectCurrentNode();
                event->accept();
                mutex.unlock();
                return;
            }
        }
        mutex.unlock();
    }
    event->ignore();
}

void
TreeCanvas::contextMenuEvent(QContextMenuEvent* event) {
    if (mutex.tryLock()) {
        VisualNode* n = eventNode(event);
        if (n != NULL) {
            setCurrentNode(n);
            emit contextMenu(event);
            event->accept();
            mutex.unlock();
            return;
        }
        mutex.unlock();
    }
    event->ignore();
}

void
TreeCanvas::resizeEvent(QResizeEvent* e) {
    QAbstractScrollArea* sa =
            static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());

    int w = sa->horizontalScrollBar()->maximum()+e->oldSize().width();
    int h = sa->verticalScrollBar()->maximum()+e->oldSize().height();

    sa->horizontalScrollBar()->setRange(0,w-e->size().width());
    sa->verticalScrollBar()->setRange(0,h-e->size().height());
    sa->horizontalScrollBar()->setPageStep(e->size().width());
    sa->verticalScrollBar()->setPageStep(e->size().height());
}

void
TreeCanvas::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ShiftModifier) {
        event->accept();
        if (event->orientation() == Qt::Vertical && !autoZoom)
            scaleTree(scale*100+ceil(static_cast<double>(event->delta())/4.0),
                      event->x(), event->y());
    } else {
        event->ignore();
    }
}

bool
TreeCanvas::finish(void) {
    if (finishedFlag)
        return true;
    stopSearchFlag = true;
    finishedFlag = true;
//    for (int i=0; i<doubleClickInspectors.size(); i++)
//        doubleClickInspectors[i].first->finalize();
//    for (int i=0; i<solutionInspectors.size(); i++)
//        solutionInspectors[i].first->finalize();
//    for (int i=0; i<moveInspectors.size(); i++)
//        moveInspectors[i].first->finalize();
//    for (int i=0; i<comparators.size(); i++)
//        comparators[i].first->finalize();
    return !ptr_receiver->isRunning();
}

void
TreeCanvas::finalizeCanvas(void) {
  qDebug() << "in finalize canvas: " << _id;
  _isUsed = true;
  ptr_receiver->updateCanvas();
}

void
TreeCanvas::setCurrentNode(VisualNode* n, bool finished, bool update) {
    if (finished)
        mutex.lock();

    if (n != NULL) {
        currentNode->setMarked(false);
        currentNode = n;
        currentNode->setMarked(true);
        emit statusChanged(currentNode,stats,finished);
        if (update) {
            compareNodes = false;
            setCursor(QCursor(Qt::ArrowCursor));
            QWidget::update();
        }
    }
    if (finished)
        mutex.unlock();
}

void
TreeCanvas::mousePressEvent(QMouseEvent* event) {
    if (mutex.tryLock()) {
        if (event->button() == Qt::LeftButton) {
            VisualNode* n = eventNode(event);
            if (compareNodes) {
                if (n != NULL && n->getStatus() != UNDETERMINED &&
                        currentNode != NULL &&
                        currentNode->getStatus() != UNDETERMINED) {
//                    Space* curSpace = NULL;
//                    Space* compareSpace = NULL;
//                    for (int i=0; i<comparators.size(); i++) {
//                        if (comparators[i].second) {
//                            if (curSpace == NULL) {
//                                curSpace = currentNode->getSpace(*na,curBest,c_d,a_d);

//                                if (!compareNodesBeforeFP || n->isRoot()) {
//                                    compareSpace = n->getSpace(*na,curBest,c_d,a_d);
//                                } else {
//                                    VisualNode* p = n->getParent(*na);
//                                    compareSpace = p->getSpace(*na,curBest,c_d,a_d);
//                                    switch (compareSpace->status()) {
//                                    case SS_SOLVED:
//                                    case SS_FAILED:
//                                        break;
//                                    case SS_BRANCH:
//                                        compareSpace->commit(*p->getChoice(),
//                                                             n->getAlternative(*na));
//                                        break;
//                                    default:
//                                        GECODE_NEVER;
//                                    }
//                                }
//                            }
//                            try {
//                                comparators[i].first->compare(*curSpace,*compareSpace);
//                            } catch (Exception& e) {
//                                qFatal("Exception in comparator %d: %s.\n Stopping.",
//                                       i, e.what());
//                            }
//                        }
//                    }
                }
            } else {
                setCurrentNode(n);
            }
            compareNodes = false;
            setCursor(QCursor(Qt::ArrowCursor));
            if (n != NULL) {
                event->accept();
                mutex.unlock();
                return;
            }
        }
        mutex.unlock();
    }
    event->ignore();
}

void
TreeCanvas::setAutoHideFailed(bool b) {
    autoHideFailed = b;
}

void
TreeCanvas::setAutoZoom(bool b) {
    autoZoom = b;
    if (autoZoom) {
        zoomToFit();
    }
    emit autoZoomChanged(b);
    scaleBar->setEnabled(!b);
}

bool
TreeCanvas::getAutoHideFailed(void) {
    return autoHideFailed;
}

bool
TreeCanvas::getAutoZoom(void) {
    return autoZoom;
}

void
TreeCanvas::setRefresh(int i) {
    refresh = i;
}

void
TreeCanvas::setRefreshPause(int i) {
    refreshPause = i;
    if (refreshPause > 0)
        refresh = 1;
}

bool
TreeCanvas::getSmoothScrollAndZoom(void) {
    return smoothScrollAndZoom;
}

void
TreeCanvas::setSmoothScrollAndZoom(bool b) {
    smoothScrollAndZoom = b;
}

bool
TreeCanvas::getMoveDuringSearch(void) {
    return moveDuringSearch;
}

void
TreeCanvas::setMoveDuringSearch(bool b) {
    moveDuringSearch = b;
}

/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "treecanvas.hh"
#include <QtGui/QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QTimer>

#include <stack>
#include <fstream>
#include <exception>
#include <ctime>

#include "zoomToFitIcon.hpp"
#include "cpprofiler/pixeltree/pixel_tree_dialog.hh"
#include "cpprofiler/pixeltree/icicle_tree_dialog.hh"
#include "highlight_nodes_dialog.hpp"
#include "nogood_dialog.hh"
#include "node_info_dialog.hh"
#include "libs/perf_helper.hh"

#include "third-party/json.hpp"

#include "data.hh"

#include "nodevisitor.hh"
#include "visualnode.hh"
#include "drawingcursor.hh"

#include "ml-stats.hh"
#include "globalhelper.hh"
#include "execution.hh"

#include <fstream>
#include <iostream>

#include <stack>

#include "cpprofiler/analysis/similar_shapes.hh"

using namespace cpprofiler::analysis;

int TreeCanvas::counter = 0;

TreeCanvas::TreeCanvas(Execution* e, QGridLayout* layout, QWidget* parent)
    : QWidget{parent},
      execution{*e},
      mutex(execution.getMutex()),
      layoutMutex(execution.getLayoutMutex()),
      na(execution.nodeTree().getNA())
  {
  QMutexLocker locker(&mutex);

  _id = TreeCanvas::counter++;

  root = execution.nodeTree().getRoot();

  m_options.scale = LayoutConfig::defScale / 100.0;

  setAutoFillBackground(true);

  QPixmap zoomPic;
  zoomPic.loadFromData(zoomToFitIcon, sizeof(zoomToFitIcon));

  auto autoZoomButton = new QToolButton(this);
  autoZoomButton->setCheckable(true);
  autoZoomButton->setIcon(zoomPic);
  autoZoomButton->setFixedSize(30, 30);
  autoZoomButton->setFocusPolicy(Qt::NoFocus);

  layout->addWidget(autoZoomButton, 1,1, Qt::AlignHCenter);

  connect(autoZoomButton, SIGNAL(toggled(bool)), this, SLOT(setAutoZoom(bool)));

  connect(this, SIGNAL(autoZoomChanged(bool)), autoZoomButton,
          SLOT(setChecked(bool)));

  connect(&execution, &Execution::newNode, this, &TreeCanvas::maybeUpdateCanvas);
  connect(&execution, &Execution::newRoot, this, &TreeCanvas::updateCanvas);

  connect(&scrollTimeLine, SIGNAL(frameChanged(int)), this, SLOT(scroll(int)));

  scrollTimeLine.setCurveShape(QTimeLine::EaseInOutCurve);

  m_scaleBar = new QSlider(Qt::Vertical, this);
  m_scaleBar->setObjectName("scaleBar");
  m_scaleBar->setMinimum(LayoutConfig::minScale);
  m_scaleBar->setMaximum(LayoutConfig::maxScale);
  m_scaleBar->setValue(LayoutConfig::defScale);
  connect(m_scaleBar, SIGNAL(valueChanged(int)), this, SLOT(scaleTree(int)));
  connect(this, SIGNAL(scaleChanged(int)), m_scaleBar, SLOT(setValue(int)));

  smallBox = new QLineEdit("100");
  smallBox->setMaximumWidth(50);
  connect(smallBox, SIGNAL(returnPressed()), this, SLOT(hideSize()));

  connect(&zoomTimeLine, SIGNAL(frameChanged(int)), m_scaleBar,
          SLOT(setValue(int)));
  zoomTimeLine.setCurveShape(QTimeLine::EaseInOutCurve);

  qRegisterMetaType<Statistics>("Statistics");

  emit needActionsUpdate(currentNode, false);

  update();

  updateTimer = new QTimer(this);
  updateTimer->setSingleShot(true);
  connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateViaTimer()));
}

TreeCanvas::~TreeCanvas() {
  if (root) {
    DisposeCursor dc(root, execution.nodeTree().getNA());
    PreorderNodeVisitor<DisposeCursor>(dc).run();
  }
}

///***********************

unsigned TreeCanvas::getTreeDepth() { return get_stats().maxDepth; }

void TreeCanvas::scaleTree(int scale0, int zoomx, int zoomy) {
  layoutMutex.lock();

  QSize viewport_size = size();
  QAbstractScrollArea* sa =
      static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());

  if (zoomx == -1) zoomx = viewport_size.width() / 2;
  if (zoomy == -1) zoomy = viewport_size.height() / 2;

  int xoff = (sa->horizontalScrollBar()->value() + zoomx) / m_options.scale;
  int yoff = (sa->verticalScrollBar()->value() + zoomy) / m_options.scale;

  BoundingBox bb;
  scale0 = std::min(std::max(scale0, LayoutConfig::minScale),
                    LayoutConfig::maxScale);
  m_options.scale = (static_cast<double>(scale0)) / 100.0;
  bb = root->getBoundingBox();
  int w = static_cast<int>((bb.right - bb.left + Layout::extent) * m_options.scale);
  int h = static_cast<int>(2 * Layout::extent +
                           root->getShape()->depth() * Layout::dist_y * m_options.scale);

  sa->horizontalScrollBar()->setRange(0, w - viewport_size.width());
  sa->verticalScrollBar()->setRange(0, h - viewport_size.height());
  sa->horizontalScrollBar()->setPageStep(viewport_size.width());
  sa->verticalScrollBar()->setPageStep(viewport_size.height());
  sa->horizontalScrollBar()->setSingleStep(Layout::extent);
  sa->verticalScrollBar()->setSingleStep(Layout::extent);

  xoff *= m_options.scale;
  yoff *= m_options.scale;

  sa->horizontalScrollBar()->setValue(xoff - zoomx);
  sa->verticalScrollBar()->setValue(yoff - zoomy);

  emit scaleChanged(scale0);
  layoutMutex.unlock();
  QWidget::update();
}

void TreeCanvas::update(void) {
  QMutexLocker locker(&mutex);
  layoutMutex.lock();
  // std::cerr << "TreeCanvas::update\n";
  if (root != nullptr) {
    // std::cerr << "root->layout\n";
    root->layout(execution.nodeTree().getNA());
    BoundingBox bb = root->getBoundingBox();

    int w = static_cast<int>((bb.right - bb.left + Layout::extent) * m_options.scale);
    int h =
        static_cast<int>(2 * Layout::extent +
                         root->getShape()->depth() * Layout::dist_y * m_options.scale);
    m_view.xtrans = -bb.left + (Layout::extent / 2);

    QSize viewport_size = size();
    QAbstractScrollArea* sa =
        static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
    sa->horizontalScrollBar()->setRange(0, w - viewport_size.width());
    sa->verticalScrollBar()->setRange(0, h - viewport_size.height());
    sa->horizontalScrollBar()->setPageStep(viewport_size.width());
    sa->verticalScrollBar()->setPageStep(viewport_size.height());
    sa->horizontalScrollBar()->setSingleStep(Layout::extent);
    sa->verticalScrollBar()->setSingleStep(Layout::extent);
  }
  if (m_options.autoZoom) zoomToFit();
  layoutMutex.unlock();
  QWidget::update();
}

void TreeCanvas::scroll(void) { QWidget::update(); }

void TreeCanvas::layoutDone(int w, int h, int scale0) {

  m_view.targetScale = scale0;

  QSize viewport_size = size();
  QAbstractScrollArea* sa =
      static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
  sa->horizontalScrollBar()->setRange(0, w - viewport_size.width());
  sa->verticalScrollBar()->setRange(0, h - viewport_size.height());

  if (layoutDoneTimerId == 0) layoutDoneTimerId = startTimer(15);
}

void TreeCanvas::statusFinished() { 
  statusChanged(true); 
}

void TreeCanvas::statusChanged(bool finished) {
  if (finished) {
    update();
    centerCurrentNode();
  }
  emit statusChanged(currentNode, get_stats(), finished);
  emit needActionsUpdate(currentNode, finished);
}

void TreeCanvas::showPixelTree(void) {
  using cpprofiler::pixeltree::PixelTreeDialog;
  auto pixelTreeDialog = new PixelTreeDialog(this);

  /// TODO(maxim): try to bypass the pt dialog
  connect(this, SIGNAL(showNodeOnPixelTree(int)), pixelTreeDialog,
          SLOT(setPixelSelected(int)));

  pixelTreeDialog->show();
}

void TreeCanvas::showIcicleTree(void) {
  using cpprofiler::pixeltree::IcicleTreeDialog;

  auto icicleTreeDialog = new IcicleTreeDialog(this);

  icicleTreeDialog->show();
}

void TreeCanvas::deleteWhiteNodes() {
  // TODO(maxim): might have to reset 'current node' if
  //              is is no longer visible
  // TODO(maxim): fix this crashing pixel tree

  // IMPORTANT(maxim): currently I leave 'holes' in *na* that
  //                   can be examined and point to nodes not visualised

  // 1. swap deleted nodes in *na* with the last in *na*
  // and remap array ids.
  qDebug() << "size before: " << na.size();
  for (auto i = 0; i < na.size(); ++i) {
    auto node = na[i];
    // TODO(maxim): play around with omitting SKIPPED nodes:
    // if (node->getStatus() == UNDETERMINED || node->getStatus() == SKIPPED) {
    if (node->getStatus() == UNDETERMINED) {
      deleteNode(node);
    }
  }
  update();
  qDebug() << "size after: " << na.size();
}

void TreeCanvas::deleteSkippedNodes() {

  for (auto i = 0; i < na.size(); ++i) {
    auto node = na[i];

    if (node->getStatus() == SKIPPED) {
      deleteNode(node);
    }
  }
  update();
}

void TreeCanvas::followPath(void) {
  QMutexLocker locker(&mutex);
  bool ok;
  QString text = QInputDialog::getText(this, tr("Path to follow"), tr("Path:"),
                                       QLineEdit::Normal, "", &ok);
  if (ok && !text.isEmpty()) {
    QStringList choices = text.split(QRegExp(";\\s+"));
    VisualNode* n = root;
    for (int i = 0; i < choices.length(); i++) {
      int numChildren = n->getNumberOfChildren();
      for (int j = 0; j < numChildren; j++) {
        int childIndex = n->getChild(j);
        VisualNode* c = execution.nodeTree().getNode(childIndex);
        // If we find the right label, follow it and go to the next
        // iteration of the outer loop.
        if (execution.nodeTree().getNA().getLabel(c) == choices[i]) {
          n = c;
          goto found;
        }
      }
      // If the inner loop terminates, we didn't find the right label.
      // In that case, terminate the outer loop.
      break;
    found:;
    }
    // However far we made it, select that node.
    setCurrentNode(n);
    centerCurrentNode();
  }
}

void TreeCanvas::analyzeSimilarSubtrees(void) {
  QMutexLocker locker_1(&mutex);
  QMutexLocker locker_2(&layoutMutex);

  shapesWindow.reset(new SimilarShapesWindow{this, execution.nodeTree()});
  shapesWindow->show();
}

void TreeCanvas::highlightNodesMenu(void) {
  auto hn_dialog = new HighlightNodesDialog(this);
  hn_dialog->show();
}

void TreeCanvas::showNogoods(void) {
  std::vector<int> selected_gids;

  GetIndexesCursor gic(currentNode, execution.nodeTree().getNA(), selected_gids);
  PreorderNodeVisitor<GetIndexesCursor>(gic).run();

  NogoodDialog* ngdialog =
      new NogoodDialog(this, *this, selected_gids, execution.getNogoods());

  ngdialog->show();
}

#ifdef MAXIM_DEBUG

std::string NA_to_string(const NodeAllocator& na) {
  std::string str = "--- NodeAllocator ---\n";

  for (auto i = 0u; i < na.size(); ++i) {
    str += std::to_string(na[i]->debug_id) + " ";
  }

  return str;
}

void print_debug(const NodeAllocator& na, const Data& data) {
  std::ofstream file("debug.txt");

  file << data.getDebugInfo();

  file << NA_to_string(na);
}

std::string boolToString(bool flag) {
  return flag ? "YES" : " - ";
}

#endif

void TreeCanvas::showNodeInfo(void) {
  auto info = execution.getInfo(*currentNode);

  std::string extra_info = (info) ? *info : "";
  extra_info += "\n";

  auto id = currentNode->getIndex(na);
  auto depth = execution.nodeTree().calculateDepth(*currentNode);

  extra_info += "--------------------------------------------\n";
  extra_info += " id: " + std::to_string(id) + "\tdepth: " + std::to_string(depth) + "\n";
  extra_info += "--------------------------------------------\n";

#ifdef MAXIM_DEBUG
  extra_info += "--------------------------------------------\n";
  extra_info += "status: " + statusToString(currentNode->getStatus()) + "\n";
  extra_info += "has open children: " + boolToString(currentNode->isOpen()) + 
                " (" + std::to_string(currentNode->getNoOfOpenChildren(na)) + ")" + "\n";
  extra_info += "has failed children: " + boolToString(currentNode->hasFailedChildren()) + "\n";
  extra_info += "has solved children: " + boolToString(currentNode->hasSolvedChildren()) + "\n";
  extra_info += "number of direct children: " + std::to_string(currentNode->getNumberOfChildren()) + "\n";
  extra_info += "--------------------------------------------\n";

  auto db_entry = getEntry(id);

  extra_info += "gecode/na id: " + std::to_string(currentNode->getIndex(na)) + "\n";
  // extra_info += "array index: " + std::to_string(db_entry) + "\n";

  assert (na[id] == currentNode);
#endif

  NodeInfoDialog* nidialog = new NodeInfoDialog(this, extra_info);
  nidialog->show();
}

void TreeCanvas::showNodeOnPixelTree(void) {
  int gid = currentNode->getIndex(execution.nodeTree().getNA());
  emit showNodeOnPixelTree(gid);
}

void TreeCanvas::collectMLStats(void) {
  ::collectMLStats(currentNode, execution.nodeTree().getNA(), &execution);
}

void TreeCanvas::collectMLStats(VisualNode* node) {
  ::collectMLStats(node, execution.nodeTree().getNA(), &execution);
}

void TreeCanvas::collectMLStatsRoot(std::ostream& out) {
  ::collectMLStats(root, execution.nodeTree().getNA(), &execution, out);
}

void TreeCanvas::highlightSubtrees(const std::vector<VisualNode*>& nodes) {
  QMutexLocker locker_1(&mutex);
  QMutexLocker locker_2(&layoutMutex);

  auto& na = execution.nodeTree().getNA();

  root->unhideAll(na);
  root->layout(na);

  UnhighlightCursor uhc(root, na);
  PreorderNodeVisitor<UnhighlightCursor>(uhc).run();

  for (auto node : nodes) {
    node->setHighlighted(true);
  }

  HideNotHighlightedCursor hnhc(root, na);
  PostorderNodeVisitor<HideNotHighlightedCursor>(hnhc).run();

  update();
}

void TreeCanvas::printPath(std::stringstream& str, const VisualNode* node) {

  str << "(node id: " << node->getIndex(na) << "):\t";

  /// use a stack to reverse the order of labels
  QStack<const VisualNode*> node_path;

  while(true) {

    node_path.push(node);

    if (node->isRoot()) break;

    node = node->getParent(na);
  }

  while(!node_path.isEmpty()) {
    auto node = node_path.pop();
    str << getLabel(node->getIndex(na)).c_str() << " ";
  }

  str << "\n";
}

void TreeCanvas::printPaths(const std::vector<VisualNode*>& nodes) {

  const auto& na = execution.nodeTree().getNA();
  std::stringstream str;

  for (auto node : nodes) {
    printPath(str, node);
  }

  Utils::writeToFile(str.str().c_str());
}

void TreeCanvas::printHightlightedPaths() {
  std::vector<VisualNode*> nodes_to_print;

  for (auto i=0; i < na.size(); ++i) {
    auto node = na[i];
    if (node->isHighlighted()) {
      nodes_to_print.push_back(node);
    }
  }

  printPaths(nodes_to_print);
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

void TreeCanvas::toggleHidden(void) {
  QMutexLocker locker(&mutex);
  currentNode->toggleHidden(execution.nodeTree().getNA());
  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

void TreeCanvas::hideFailed(void) {
  QMutexLocker locker(&mutex);
  currentNode->hideFailed(execution.nodeTree().getNA());
  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

void TreeCanvas::hideSize() {
  QMutexLocker locker(&mutex);
  QString boxContents = smallBox->text();
  int threshold = boxContents.toInt();
  currentNode->hideSize(threshold, execution.nodeTree().getNA());
  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

void TreeCanvas::hideAll(void) {
  QMutexLocker locker_1(&mutex);
  QMutexLocker locker_2(&layoutMutex);

  HideAllCursor hac(root, execution.nodeTree().getNA());
  PostorderNodeVisitor<HideAllCursor>(hac).run();

  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

void TreeCanvas::unhideAll(void) {
  QMutexLocker locker(&mutex);
  QMutexLocker layoutLocker(&layoutMutex);
  currentNode->unhideAll(execution.nodeTree().getNA());
  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

void TreeCanvas::unselectAll(void) {
  QMutexLocker locker(&mutex);
  QMutexLocker layoutLocker(&layoutMutex);
  root->unselectAll(execution.nodeTree().getNA());
  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

void TreeCanvas::unhideNode(VisualNode* node) {
  node->dirtyUp(execution.nodeTree().getNA());

  auto* next = node;
  do {
    next->setHidden(false);
  } while ((next = next->getParent(execution.nodeTree().getNA())));
}

void TreeCanvas::toggleStop(void) {
  QMutexLocker locker(&mutex);
  currentNode->toggleStop(execution.nodeTree().getNA());
  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

void TreeCanvas::unstopAll(void) {
  QMutexLocker locker(&mutex);
  QMutexLocker layoutLocker(&layoutMutex);
  currentNode->unstopAll(execution.nodeTree().getNA());
  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

void TreeCanvas::timerEvent(QTimerEvent* e) {
  if (e->timerId() == layoutDoneTimerId) {
    if (!m_options.smoothScrollAndZoom) {
      scaleTree(m_view.targetScale);
    } else {
      zoomTimeLine.stop();
      int zoomCurrent = static_cast<int>(m_options.scale * 100);
      int targetZoom = std::min(std::max(m_view.targetScale, LayoutConfig::minScale),
                            LayoutConfig::maxAutoZoomScale);
      zoomTimeLine.setFrameRange(zoomCurrent, targetZoom);
      zoomTimeLine.start();
    }
    QWidget::update();
    killTimer(layoutDoneTimerId);
    layoutDoneTimerId = 0;
  }
}

void TreeCanvas::zoomToFit(void) {
  QMutexLocker locker(&layoutMutex);
  if (root != nullptr) {
    BoundingBox bb;
    bb = root->getBoundingBox();
    QWidget* p = parentWidget();
    if (p) {
      double newXScale = static_cast<double>(p->width()) /
                         (bb.right - bb.left + Layout::extent);
      double newYScale =
          static_cast<double>(p->height()) /
          (root->getShape()->depth() * Layout::dist_y + 2 * Layout::extent);
      int scale0 = static_cast<int>(std::min(newXScale, newYScale) * 100);
      if (scale0 < LayoutConfig::minScale) scale0 = LayoutConfig::minScale;
      if (scale0 > LayoutConfig::maxAutoZoomScale)
        scale0 = LayoutConfig::maxAutoZoomScale;

      if (!m_options.smoothScrollAndZoom) {
        scaleTree(scale0);
      } else {
        zoomTimeLine.stop();
        int zoomCurrent = static_cast<int>(m_options.scale * 100);
        int targetZoom = scale0;
        targetZoom = std::min(std::max(targetZoom, LayoutConfig::minScale),
                              LayoutConfig::maxAutoZoomScale);
        zoomTimeLine.setFrameRange(zoomCurrent, targetZoom);
        zoomTimeLine.start();
      }
    }
  }
}

void TreeCanvas::centerCurrentNode(void) {
  QMutexLocker locker(&mutex);
  int x = 0;
  int y = 0;

  VisualNode* c = currentNode;
  while (c != nullptr) {
    x += c->getOffset();
    y += Layout::dist_y;
    c = c->getParent(execution.nodeTree().getNA());
  }

  x = static_cast<int>((m_view.xtrans + x) * m_options.scale);
  y = static_cast<int>(y * m_options.scale);

  QAbstractScrollArea* sa =
      static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());

  x -= sa->viewport()->width() / 2;
  y -= sa->viewport()->height() / 2;

  m_view.sourceX = sa->horizontalScrollBar()->value();
  m_view.targetX = std::max(sa->horizontalScrollBar()->minimum(), x);
  m_view.targetX = std::min(sa->horizontalScrollBar()->maximum(), m_view.targetX);
  m_view.sourceY = sa->verticalScrollBar()->value();
  m_view.targetY = std::max(sa->verticalScrollBar()->minimum(), y);
  m_view.targetY = std::min(sa->verticalScrollBar()->maximum(), m_view.targetY);
  if (!m_options.smoothScrollAndZoom) {
    sa->horizontalScrollBar()->setValue(m_view.targetX);
    sa->verticalScrollBar()->setValue(m_view.targetY);
  } else {
    scrollTimeLine.stop();
    scrollTimeLine.setFrameRange(0, 100);
    scrollTimeLine.setDuration(
        std::max(200, std::min(1000, std::min(std::abs(m_view.sourceX - m_view.targetX),
                                              std::abs(m_view.sourceY - m_view.targetY)))));
    scrollTimeLine.start();
  }
}

void TreeCanvas::scroll(int i) {
  QAbstractScrollArea* sa =
      static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
  double p = static_cast<double>(i) / 100.0;
  double xdiff = static_cast<double>(m_view.targetX - m_view.sourceX) * p;
  double ydiff = static_cast<double>(m_view.targetY - m_view.sourceY) * p;
  sa->horizontalScrollBar()->setValue(m_view.sourceX + static_cast<int>(xdiff));
  sa->verticalScrollBar()->setValue(m_view.sourceY + static_cast<int>(ydiff));
}

/// check what should be uncommented out.
void TreeCanvas::expandCurrentNode() {
  QMutexLocker locker(&mutex);

  if (currentNode->isHidden()) {
    toggleHidden();
    return;
  }

  if (currentNode->getStatus() == MERGING) {
    toggleHidden();

    return;
  }

  currentNode->dirtyUp(execution.nodeTree().getNA());
  update();
}

void TreeCanvas::labelBranches(void) {
  QMutexLocker locker(&mutex);
  currentNode->labelBranches(execution.nodeTree().getNA(), *this);
  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}
void TreeCanvas::labelPath(void) {
  QMutexLocker locker(&mutex);
  currentNode->labelPath(execution.nodeTree().getNA(), *this);
  update();
  centerCurrentNode();
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

/// TODO(maxim): this should not not re-build a tree, disabled for now
/// (it is still called from GistMainWidnow)
void TreeCanvas::reset() {
  QMutexLocker locker(&mutex);

  VisualNode* root = execution.nodeTree().getRoot();
  currentNode = root;
  m_options.scale = 1.0;
  for (int i = bookmarks.size(); i--;) emit removedBookmark(i);
  bookmarks.clear();

  // _builder->reset(execution, &execution->getNA());
  // _builder->start();

  emit statusChanged(currentNode, get_stats(), false);

  updateCanvas();
}

void TreeCanvas::bookmarkNode(void) {
  QMutexLocker locker(&mutex);
  if (!currentNode->isBookmarked()) {
    bool ok;
    QString text = QInputDialog::getText(this, "Add bookmark", "Name:",
                                         QLineEdit::Normal, "", &ok);
    if (ok) {
      currentNode->setBookmarked(true);
      bookmarks.append(currentNode);
      if (text == "")
        text = QString("Node ") + QString().setNum(bookmarks.size());
      emit addedBookmark(text);
    }
  } else {
    currentNode->setBookmarked(false);
    int idx = bookmarks.indexOf(currentNode);
    bookmarks.remove(idx);
    emit removedBookmark(idx);
  }
  currentNode->dirtyUp(execution.nodeTree().getNA());
  update();
}

void TreeCanvas::emitStatusChanged(void) {
  emit statusChanged(currentNode, get_stats(), true);
  emit needActionsUpdate(currentNode, true);
}

void TreeCanvas::navUp(void) {
  QMutexLocker locker(&mutex);
  VisualNode* p = currentNode->getParent(execution.nodeTree().getNA());

  setCurrentNode(p);

  if (p != nullptr) {
    centerCurrentNode();
  }
}

void TreeCanvas::navDown(void) {
  QMutexLocker locker(&mutex);
  if (!currentNode->isHidden()) {
    switch (currentNode->getStatus()) {
      case STOP:
      case UNSTOP:
      case MERGING:
      case BRANCH: {
        int alt = std::max(0, currentNode->getPathAlternative(execution.nodeTree().getNA()));
        VisualNode* n = currentNode->getChild(execution.nodeTree().getNA(), alt);
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

void TreeCanvas::navLeft(void) {
  QMutexLocker locker(&mutex);
  VisualNode* p = currentNode->getParent(execution.nodeTree().getNA());
  if (p != nullptr) {
    int alt = currentNode->getAlternative(execution.nodeTree().getNA());
    if (alt > 0) {
      VisualNode* n = p->getChild(execution.nodeTree().getNA(), alt - 1);
      setCurrentNode(n);
      centerCurrentNode();
    }
  }
}

void TreeCanvas::navRight(void) {
  QMutexLocker locker(&mutex);
  VisualNode* p = currentNode->getParent(execution.nodeTree().getNA());
  if (p != nullptr) {
    uint alt = currentNode->getAlternative(execution.nodeTree().getNA());
    if (alt + 1 < p->getNumberOfChildren()) {
      VisualNode* n = p->getChild(execution.nodeTree().getNA(), alt + 1);
      setCurrentNode(n);
      centerCurrentNode();
    }
  }
}

void TreeCanvas::navRoot(void) {
  QMutexLocker locker(&mutex);
  setCurrentNode(root);
  centerCurrentNode();
}

void TreeCanvas::navNextSol(bool back) {
  QMutexLocker locker(&mutex);
  NextSolCursor nsc(currentNode, back, execution.nodeTree().getNA());
  PreorderNodeVisitor<NextSolCursor> nsv(nsc);
  nsv.run();
  VisualNode* n = nsv.getCursor().node();
  if (n != root) {
    setCurrentNode(n);
    centerCurrentNode();
  }
}

void TreeCanvas::navNextLeaf(bool back) {
  QMutexLocker locker(&mutex);
  NextLeafCursor nsc(currentNode, back, execution.nodeTree().getNA());
  PreorderNodeVisitor<NextLeafCursor> nsv(nsc);
  nsv.run();
  VisualNode* n = nsv.getCursor().node();
  if (n != root) {
    setCurrentNode(n);
    centerCurrentNode();
  }
}

void TreeCanvas::navNextPentagon(bool back) {
  QMutexLocker locker(&mutex);
  NextPentagonCursor nsc(currentNode, back, execution.nodeTree().getNA());
  PreorderNodeVisitor<NextPentagonCursor> nsv(nsc);
  nsv.run();
  VisualNode* n = nsv.getCursor().node();
  if (n != root) {
    setCurrentNode(n);
    centerCurrentNode();
  }
}

void TreeCanvas::navPrevSol(void) { navNextSol(true); }
void TreeCanvas::navPrevLeaf(void) { navNextLeaf(true); }

void TreeCanvas::exportNodePDF(VisualNode* n) {

  QString filename = QFileDialog::getSaveFileName(
      this, tr("Export tree as pdf"), "", tr("PDF (*.pdf)"));

  if (filename == "") return;

  QPrinter printer(QPrinter::ScreenResolution);
  QMutexLocker locker(&mutex);

  BoundingBox bb = n->getBoundingBox();
  printer.setFullPage(true);
  printer.setPaperSize(
      QSizeF(bb.right - bb.left + Layout::extent,
             n->getShape()->depth() * Layout::dist_y + Layout::extent),
      QPrinter::Point);
  printer.setOutputFileName(filename);
  QPainter painter(&printer);

  painter.setRenderHint(QPainter::Antialiasing);

  QRect pageRect = printer.pageRect();
  double newXScale = static_cast<double>(pageRect.width()) /
                     (bb.right - bb.left + Layout::extent);
  double newYScale =
      static_cast<double>(pageRect.height()) /
      (n->getShape()->depth() * Layout::dist_y + Layout::extent);
  double printScale = std::min(newXScale, newYScale);
  painter.scale(printScale, printScale);

  int printxtrans = -bb.left + (Layout::extent / 2);

  painter.translate(printxtrans, Layout::dist_y / 2);
  QRect clip(0, 0, 0, 0);
  DrawingCursor dc(n, execution.nodeTree().getNA(), painter, clip);
  currentNode->setMarked(false);
  PreorderNodeVisitor<DrawingCursor>(dc).run();
  currentNode->setMarked(true);

}

void TreeCanvas::exportWholeTreePDF(void) {
#if QT_VERSION >= 0x040400
  exportNodePDF(root);
#endif
}

void TreeCanvas::exportPDF(void) {
#if QT_VERSION >= 0x040400
  exportNodePDF(currentNode);
#endif
}

void TreeCanvas::print(void) {
  QPrinter printer;
  if (QPrintDialog(&printer, this).exec() == QDialog::Accepted) {
    QMutexLocker locker(&mutex);

    BoundingBox bb = root->getBoundingBox();
    QRect pageRect = printer.pageRect();
    double newXScale = static_cast<double>(pageRect.width()) /
                       (bb.right - bb.left + Layout::extent);
    double newYScale =
        static_cast<double>(pageRect.height()) /
        (root->getShape()->depth() * Layout::dist_y + 2 * Layout::extent);
    double printScale = std::min(newXScale, newYScale) * 100;
    if (printScale < 1.0) printScale = 1.0;
    if (printScale > 400.0) printScale = 400.0;
    printScale = printScale / 100.0;

    QPainter painter(&printer);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(printScale, printScale);
    painter.translate(m_view.xtrans, 0);
    QRect clip(0, 0, 0, 0);
    DrawingCursor dc(root, execution.nodeTree().getNA(), painter, clip);
    PreorderNodeVisitor<DrawingCursor>(dc).run();
  }
}

void TreeCanvas::printSearchLog(void) {

  std::stringstream out;
  SearchLogCursor slc(root, out, execution.nodeTree().getNA(), execution);
  PreorderNodeVisitor<SearchLogCursor>(slc).run();

  std::cout << "SEARCH LOG READY" << std::endl;

  Utils::writeToFile(out.str().c_str());
}

VisualNode* TreeCanvas::eventNode(QEvent* event) {
  int x = 0;
  int y = 0;
  switch (event->type()) {
    case QEvent::ToolTip: {
      QHelpEvent* he = static_cast<QHelpEvent*>(event);
      x = he->x();
      y = he->y();
      break;
    }
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove: {
      QMouseEvent* me = static_cast<QMouseEvent*>(event);
      x = me->x();
      y = me->y();
      break;
    }
    case QEvent::ContextMenu: {
      QContextMenuEvent* ce = static_cast<QContextMenuEvent*>(event);
      x = ce->x();
      y = ce->y();
      break;
    }
    default:
      return nullptr;
  }
  QAbstractScrollArea* sa =
      static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
  int xoff = sa->horizontalScrollBar()->value() / m_options.scale;
  int yoff = sa->verticalScrollBar()->value() / m_options.scale;

  BoundingBox bb = root->getBoundingBox();
  int w = static_cast<int>((bb.right - bb.left + Layout::extent) * m_options.scale);
  if (w < sa->viewport()->width()) xoff -= (sa->viewport()->width() - w) / 2;

  VisualNode* n;
  n = root->findNode(execution.nodeTree().getNA(), static_cast<int>(x / m_options.scale - m_view.xtrans + xoff),
                     static_cast<int>((y - 30) / m_options.scale + yoff));
  return n;
}

bool TreeCanvas::event(QEvent* event) {
  if (mutex.tryLock()) {
    if (event->type() == QEvent::ToolTip) {
      VisualNode* n = eventNode(event);
      if (n != nullptr) {
        QHelpEvent* he = static_cast<QHelpEvent*>(event);
        QToolTip::showText(he->globalPos(), QString(n->toolTip(execution.nodeTree().getNA()).c_str()));
      } else {
        QToolTip::hideText();
      }
    }
    mutex.unlock();
  }
  return QWidget::event(event);
}

void TreeCanvas::resizeToOuter(void) {
  if (m_options.autoZoom) zoomToFit();
}

void TreeCanvas::paintEvent(QPaintEvent* event) {
  QMutexLocker locker(&layoutMutex);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  QAbstractScrollArea* sa =
      static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());
  int xoff = sa->horizontalScrollBar()->value() / m_options.scale;
  int yoff = sa->verticalScrollBar()->value() / m_options.scale;

  BoundingBox bb = root->getBoundingBox();
  int w = static_cast<int>((bb.right - bb.left + Layout::extent) * m_options.scale);
  if (w < sa->viewport()->width()) xoff -= (sa->viewport()->width() - w) / 2;

  QRect origClip = event->rect();
  painter.translate(0, 30);
  painter.scale(m_options.scale, m_options.scale);
  painter.translate(m_view.xtrans - xoff, -yoff);
  QRect clip(static_cast<int>(origClip.x() / m_options.scale - m_view.xtrans + xoff),
             static_cast<int>(origClip.y() / m_options.scale + yoff),
             static_cast<int>(origClip.width() / m_options.scale),
             static_cast<int>(origClip.height() / m_options.scale));

  // perfHelper.begin("TreeCanvas: paint");
  DrawingCursor dc(root, execution.nodeTree().getNA(), painter, clip);
  PreorderNodeVisitor<DrawingCursor>(dc).run();
  // perfHelper.end();
  // int nodesLayouted = 1;
  // clock_t t0 = clock();
  // while (v.next()) { nodesLayouted++; }
  // double t = (static_cast<double>(clock()-t0) / CLOCKS_PER_SEC) * 1000.0;
  // double nps = static_cast<double>(nodesLayouted) /
  //   (static_cast<double>(clock()-t0) / CLOCKS_PER_SEC);
  // std::cout << "Drawing done. " << nodesLayouted << " nodes in "
  //   << t << " ms. " << nps << " nodes/s." << std::endl;
}

void TreeCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
  if (mutex.tryLock()) {
    if (event->button() == Qt::LeftButton) {
      VisualNode* n = eventNode(event);
      if (n == currentNode) {
        expandCurrentNode();
        event->accept();
        mutex.unlock();
        return;
      }
    }
    mutex.unlock();
  }
  event->ignore();
}

void TreeCanvas::contextMenuEvent(QContextMenuEvent* event) {
  if (mutex.tryLock()) {
    VisualNode* n = eventNode(event);
    if (n != nullptr) {
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

void TreeCanvas::resizeEvent(QResizeEvent* e) {
  QAbstractScrollArea* sa =
      static_cast<QAbstractScrollArea*>(parentWidget()->parentWidget());

  int w = sa->horizontalScrollBar()->maximum() + e->oldSize().width();
  int h = sa->verticalScrollBar()->maximum() + e->oldSize().height();

  sa->horizontalScrollBar()->setRange(0, w - e->size().width());
  sa->verticalScrollBar()->setRange(0, h - e->size().height());
  sa->horizontalScrollBar()->setPageStep(e->size().width());
  sa->verticalScrollBar()->setPageStep(e->size().height());
}

void TreeCanvas::wheelEvent(QWheelEvent* event) {
  if (event->modifiers() & Qt::ShiftModifier) {
    event->accept();
    if (event->orientation() == Qt::Vertical && !m_options.autoZoom)
      scaleTree(m_options.scale * 100 + ceil(static_cast<double>(event->delta()) / 4.0),
                event->x(), event->y());
  } else {
    event->ignore();
  }
}

void TreeCanvas::finalizeCanvas(void) {
  statusChanged(true);
}

void TreeCanvas::setCurrentNode(VisualNode* n, bool finished, bool update) {
  if (finished) mutex.lock();

  if (n != nullptr) {
    if (currentNode) {
      currentNode->setMarked(false);
    }
    bool changed = (n != currentNode);
    currentNode = n;
    currentNode->setMarked(true);
    emit statusChanged(currentNode, get_stats(), finished);
    emit needActionsUpdate(currentNode, finished);
    if (changed) {
        emit announceSelectNode(n->getIndex(execution.nodeTree().getNA()));
    }
    if (update) {
      setCursor(QCursor(Qt::ArrowCursor));
      QWidget::update();
    }
  }
  if (finished) mutex.unlock();
}

void TreeCanvas::navigateToNodeById(int gid) {
  QMutexLocker locker(&mutex);

  VisualNode* node = (execution.nodeTree().getNA())[gid];

  setCurrentNode(node, true, true);

  UnhideAncestorsCursor unhideCursor(node, execution.nodeTree().getNA());
  AncestorNodeVisitor<UnhideAncestorsCursor> unhideAncestors(unhideCursor);
  unhideAncestors.run();

  centerCurrentNode();
  update();
}

void TreeCanvas::mousePressEvent(QMouseEvent* event) {
  if (mutex.tryLock()) {
    if (event->button() == Qt::LeftButton) {
      VisualNode* n = eventNode(event);
      setCurrentNode(n);
      setCursor(QCursor(Qt::ArrowCursor));
      if (n != nullptr) {
        event->accept();
        mutex.unlock();
        return;
      }
    }
    mutex.unlock();
  }
  event->ignore();
}

void TreeCanvas::setAutoHideFailed(bool b) { m_options.autoHideFailed = b; }

void TreeCanvas::setAutoZoom(bool b) {
  m_options.autoZoom = b;
  if (m_options.autoZoom) {
    zoomToFit();
  }
  emit autoZoomChanged(b);
  m_scaleBar->setEnabled(!b);
}

bool TreeCanvas::getAutoHideFailed(void) { return m_options.autoHideFailed; }

bool TreeCanvas::getAutoZoom(void) { return m_options.autoZoom; }

void TreeCanvas::setRefresh(int i) { m_options.refreshRate = i; }

void TreeCanvas::setRefreshPause(int i) {
  m_options.refreshPause = i;
  if (m_options.refreshPause > 0) m_options.refreshRate = 1;
}

bool TreeCanvas::getSmoothScrollAndZoom(void) { return m_options.smoothScrollAndZoom; }

void TreeCanvas::setSmoothScrollAndZoom(bool b) { m_options.smoothScrollAndZoom = b; }

bool TreeCanvas::getMoveDuringSearch(void) { return m_options.moveDuringSearch; }

void TreeCanvas::setMoveDuringSearch(bool b) { m_options.moveDuringSearch = b; }

// Call this when there is a new node, and the canvas will update if
// the refresh rate says that it should.
void TreeCanvas::maybeUpdateCanvas(void) {
  nodeCount++;
  if (nodeCount >= m_options.refreshRate) {
    nodeCount = 0;
    updateCanvas();
  } else {
    // Didn't update this time: set/update timer to update in
    // 100ms.
    updateTimer->start(1000);
  }
}

void TreeCanvas::updateViaTimer(void) {
  qDebug() << "update via timer";
  nodeCount = 0;
  updateCanvas();
}

void TreeCanvas::updateCanvas(void) {
  statusChanged(false);

  QMutexLocker locker1(&mutex);
  QMutexLocker locker2(&layoutMutex);

  if (root == nullptr) return;

  if (m_options.autoHideFailed) {
    root->hideFailed(execution.nodeTree().getNA(), true);
  }

  for (VisualNode* n = currentNode; n != nullptr; n = n->getParent(execution.nodeTree().getNA())) {
    if (n->isHidden()) {
      currentNode->setMarked(false);
      currentNode = n;
      currentNode->setMarked(true);
      break;
    }
  }

  root->layout(execution.nodeTree().getNA());
  BoundingBox bb = root->getBoundingBox();

  int w = static_cast<int>((bb.right - bb.left + Layout::extent) * m_options.scale);
  int h = static_cast<int>(2 * Layout::extent +
                           root->getShape()->depth() * Layout::dist_y * m_options.scale);
  m_view.xtrans = -bb.left + (Layout::extent / 2);

  int scale0 = static_cast<int>(m_options.scale * 100);
  if (m_options.autoZoom) {
    QWidget* p = parentWidget();
    if (p) {
      double newXScale = static_cast<double>(p->width()) /
                         (bb.right - bb.left + Layout::extent);
      double newYScale =
          static_cast<double>(p->height()) /
          (root->getShape()->depth() * Layout::dist_y + 2 * Layout::extent);

      scale0 = static_cast<int>(std::min(newXScale, newYScale) * 100);
      if (scale0 < LayoutConfig::minScale) scale0 = LayoutConfig::minScale;
      if (scale0 > LayoutConfig::maxAutoZoomScale)
        scale0 = LayoutConfig::maxAutoZoomScale;
      double scale = (static_cast<double>(scale0)) / 100.0;

      w = static_cast<int>((bb.right - bb.left + Layout::extent) * m_options.scale);
      h = static_cast<int>(2 * Layout::extent +
                           root->getShape()->depth() * Layout::dist_y * m_options.scale);
    }
  }

  update();
  layoutDone(w, h, scale0);
  // emit update(w,h,scale0);
}

void TreeCanvas::applyToEachNodeIf(std::function<void(VisualNode*)> action,
                                   std::function<bool(VisualNode*)> predicate) {
  for (int i = 0; i < execution.nodeTree().getNA().size(); ++i) {
    VisualNode* node = (execution.nodeTree().getNA())[i];

    if (predicate(node)) {
      action(node);
    }
  }
}

/// Post-Order traversal
void TreeCanvas::applyToEachNodePO(std::function<void(VisualNode*)> action) {

  /// PO-traversal requires two stacks
  std::stack<VisualNode*> nodeStack1;
  std::stack<VisualNode*> nodeStack2;

  nodeStack1.push(root);

  while (nodeStack1.size() > 0) {
    VisualNode* node = nodeStack1.top();
    nodeStack1.pop();

    nodeStack2.push(node);

    auto kids = node->getNumberOfChildren();
    for (auto i = 0u; i < kids; ++i) {
      auto kid = node->getChild(na, i);
      nodeStack1.push(kid);
    }
  }

  while (nodeStack2.size() > 0) {
    VisualNode* node = nodeStack2.top();
    nodeStack2.pop();

    action(node);
  }

}

void unhighlightAllNodes(NodeAllocator& na) {
  for (auto i = 0; i < na.size(); ++i) {
    na[i]->setHovered(false);
  }
}

void TreeCanvas::resetNodesHighlighting() {
  unhighlightAllNodes(execution.nodeTree().getNA());

  update();
}

void TreeCanvas::highlightNodesWithInfo() {
  /// TODO(maxim): unhighlight all nodes first
  unhighlightAllNodes(execution.nodeTree().getNA());

  auto action = [](VisualNode* node) { node->setHovered(true); };

  /// Does the node have non-empty info field?
  auto predicate = [this](VisualNode* node) {

    auto info = execution.getInfo(*node);

    if (!info) {
      return false;
    }

    return true;
  };

  applyToEachNodeIf(action, predicate);

  update();
}

void TreeCanvas::highlightFailedByNogoods() {
  /// TODO(maxim): unhighlight all nodes first
  unhighlightAllNodes(execution.nodeTree().getNA());

  auto action = [](VisualNode* node) { node->setHovered(true); };

  auto predicate = [this](VisualNode* node) {
    auto info = execution.getInfo(*node);

    if (!info) return false;

    auto info_json = nlohmann::json::parse(*info);

    auto nogoods = info_json["nogoods"];

    if (nogoods.is_array() && nogoods.size() > 0) return true;

    return false;

  };

  applyToEachNodeIf(action, predicate);

  update();
}

void TreeCanvas::deleteNode(Node* n) {
  auto parent = n->getParent(na);
  if (!parent) return;
  parent->removeChild(n->getIndex(na), na);
  parent->dirtyUp(na);
}


using namespace std;

/// Update stats based on the status of n
static void updateStats(Statistics& stats, const VisualNode* n) {
  auto status = n->getStatus();

  switch (status) {
    case BRANCH:
      ++stats.choices;
      break;
    case SOLVED:
      ++stats.solutions;
      break;
    case FAILED: /// meant to fall through
    case SKIPPED:
      ++stats.failures;
      break;
    case UNDETERMINED:
      ++stats.undetermined;
      break;
    default:
      qDebug() << "handle this case";
  }
}

/// TODO(maxim): sometimes the new canvas is lagging (blocked by a mutex?)
static void copyTree(VisualNode* target, NodeTree& tree_target,
                     Data& data_target, const VisualNode* source,
                     const NodeTree& tree_source, const Data& data_source) {
  /// assumes that target is not assigned a number of children

  auto& stats = tree_target.getStatistics();
  stats.undetermined = 0; /// default is 1

  std::stack<VisualNode*> target_stack;
  std::stack<const VisualNode*> source_stack;

  target_stack.push(target);
  source_stack.push(source);

  int depth = 0;

  while (source_stack.size() > 0) {

    /// TODO(maxim): isn't this info readily available somewhere?
    depth = (source_stack.size() > depth) ? source_stack.size() : depth;

    VisualNode* target = target_stack.top(); target_stack.pop();
    const VisualNode* source = source_stack.top(); source_stack.pop();

    auto no_kids = source->getNumberOfChildren();
    target->setNumberOfChildren(no_kids, tree_target.getNA());
    target->nstatus = source->nstatus;

    target->dirtyUp(tree_target.getNA());

    auto src_entry = data_source.getEntry(tree_source.getIndex(source));
    auto target_index = tree_target.getIndex(target);
    data_target.connectNodeToEntry(target_index, src_entry);

    updateStats(stats, source);

    for (auto i = 0u; i < no_kids; ++i) {
      target_stack.push(tree_target.getChild(*target, i));
      source_stack.push(tree_source.getChild(*source, i));
    }

  }

  stats.maxDepth = depth;
}

/// TODO(maxim): shouldn't really be a part of TreeCanvas
pair<unique_ptr<NodeTree>, unique_ptr<Data>>
TreeCanvas::extractSubtree() {
  
  /// populate this
  unique_ptr<NodeTree> nt{new NodeTree};
  unique_ptr<Data> data{new Data};

  data->setTitle("Extracted Subtree");

  auto root = nt->getRoot();

  copyTree(root, *nt, *data, currentNode, execution.nodeTree(),
           *execution.getData());

  return make_pair(std::move(nt), std::move(data));
}

#ifdef MAXIM_DEBUG
void TreeCanvas::printDebugInfo() {
  print_debug(na, *execution.getData());
  qDebug() << "debug info recorded into debug.txt";
}

void TreeCanvas::addChildren() {
  if (currentNode->getNumberOfChildren() == 0) {
    currentNode->setNumberOfChildren(2, na);
    currentNode->dirtyUp(na);

    /// update data entry for this node

    // stats.maxDepth = std::max(stats.maxDepth, static_cast<int>(dbEntry.depth));
  }
  
  updateCanvas();
}

void TreeCanvas::deleteSelectedNode() {
  currentNode->dirtyUp(na);
  auto parent = currentNode->getParent(na);
  deleteNode(currentNode);

  setCurrentNode(parent, false, false);
  updateCanvas();
}

void TreeCanvas::dirtyUpNode() {
  currentNode->dirtyUp(na);
  updateCanvas();
}


#endif


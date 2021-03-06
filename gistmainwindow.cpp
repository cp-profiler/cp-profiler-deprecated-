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

#include <QLabel>
#include "profiler-conductor.hh"

#include "gistmainwindow.h"
#include "nodestats.hh"
#include "preferences.hh"
#include "nodewidget.hh"
#include "treecanvas.hh"
#include "data.hh"
#include "nodevisitor.hh"
#include "nodecursor.hh"
#include "zoomToFitIcon.hpp"

#include "treecanvas.hh"
#include "execution.hh"

#include <cmath>
#include <fstream>

#include <QAction>
#include <QMutexLocker>

class NodeStatsBar : public QWidget {
  /// Status bar label for maximum depth indicator
  QLabel* depthLabel;
  /// Status bar label for number of solutions
  QLabel* solvedLabel;
  /// Status bar label for number of failures
  QLabel* failedLabel;
  /// Status bar label for number of choices
  QLabel* choicesLabel;
  /// Status bar label for number of open nodes
  QLabel* openLabel;

public:

  NodeStatsBar() {
    QHBoxLayout* hbl = new QHBoxLayout{this};
    hbl->setContentsMargins(0, 0, 0, 0);

    hbl->addWidget(new QLabel("Depth:"));
    depthLabel = new QLabel("0");
    hbl->addWidget(depthLabel);

    hbl->addWidget(new NodeWidget(SOLVED));
    solvedLabel = new QLabel("0");
    hbl->addWidget(solvedLabel);

    hbl->addWidget(new NodeWidget(FAILED));
    failedLabel = new QLabel("0");
    hbl->addWidget(failedLabel);

    hbl->addWidget(new NodeWidget(BRANCH));
    choicesLabel = new QLabel("0");
    hbl->addWidget(choicesLabel);

    hbl->addWidget(new NodeWidget(UNDETERMINED));
    openLabel = new QLabel("0");
    hbl->addWidget(openLabel);
  }

  void display(const Statistics& stats) {
    depthLabel->setNum(stats.maxDepth);
    solvedLabel->setNum(stats.solutions);
    failedLabel->setNum(stats.failures);
    choicesLabel->setNum(stats.choices);
    openLabel->setNum(stats.undetermined);
  }
};

GistMainWindow::GistMainWindow(Execution& e,
                               ProfilerConductor* conductor)
    : QMainWindow(dynamic_cast<QMainWindow*>(conductor)),
      conductor(*conductor),
      execution(e),
      m_NodeStatsBar{new NodeStatsBar{}}
{
  QMutexLocker locker(&gistMutex);

  qDebug() << "Gist in thread: " << QThread::currentThreadId();

  resize(500,500);
  setMinimumSize(400, 200);

  setWindowTitle(tr("CP-Profiler"));
  statusBar()->showMessage("Ready");

  layout = new QGridLayout();

  auto main_widget = new QWidget{};
  main_widget->setLayout(layout);
  setCentralWidget(main_widget);

  m_Canvas.reset(new TreeCanvas(&execution));

  layout->addWidget(m_Canvas->scrollArea(), 0, 0, -1, 1);
  layout->addWidget(m_Canvas->scaleBar(), 1, 1, Qt::AlignHCenter);

  {
    QPixmap zoomPic;
    zoomPic.loadFromData(zoomToFitIcon, sizeof(zoomToFitIcon));

    auto autoZoomButton = new QToolButton(this);
    autoZoomButton->setCheckable(true);
    autoZoomButton->setIcon(zoomPic);
    autoZoomButton->setFixedSize(30, 30);
    autoZoomButton->setFocusPolicy(Qt::NoFocus);

    layout->addWidget(autoZoomButton, 0,1, Qt::AlignHCenter);

    connect(autoZoomButton, SIGNAL(toggled(bool)), m_Canvas.get(), SLOT(setAutoZoom(bool)));

    connect(m_Canvas.get(), SIGNAL(autoZoomChanged(bool)), autoZoomButton,
            SLOT(setChecked(bool)));
  }

  {
    /// Box for selecting "small subtree" size
    auto smallBox = new QLineEdit("100");
    smallBox->setMaximumWidth(50);
    connect(smallBox, &QLineEdit::textChanged, m_Canvas.get(), &TreeCanvas::hideSize);
    layout->addWidget(smallBox, 2, 1, Qt::AlignHCenter);
  }

  connect(&e.nodeTree(), &NodeTree::treeModified, [this]() {
    m_Canvas->updateCanvas(false);
  });

  connect(m_Canvas.get(), &TreeCanvas::nodeSelected, this, &GistMainWindow::updateActions);

  // TODO(maxim): this won't be called is Gist doesn't exist yet
  connect(&execution, &Execution::doneBuilding, this, &GistMainWindow::finishStatsBar);

  connect(m_Canvas.get(), &TreeCanvas::moreNodesDrawn, this, &GistMainWindow::updateStatsBar);

  /// in case the above was too late
  if (execution.getData().isDone()) {
    qDebug() << "too late!";
    updateStatsBar();
    finishStatsBar();
  }


//  Logos logos;
//  QPixmap myPic;
//  myPic.loadFromData(logos.gistLogo, logos.gistLogoSize);
//  setWindowIcon(myPic);

  menuBar = new QMenuBar(0);

  bookmarksMenu = new QMenu("Bookmarks", this);

  addActions();

  {
    printSearchLog = new QAction("Export search log...", this);
    addAction(printSearchLog);
    connect(printSearchLog, SIGNAL(triggered()), m_Canvas.get(), SLOT(printSearchLog()));

    QMenu* fileMenu = menuBar->addMenu(tr("&File"));
    fileMenu->addAction(print);
    fileMenu->addAction(printSearchLog);
    fileMenu->addAction(exportWholeTreePDF);

    prefAction = fileMenu->addAction(tr("Preferences"));
    connect(prefAction, SIGNAL(triggered()), this, SLOT(preferences()));
  
    QAction* quitAction = fileMenu->addAction(tr("Quit"));
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
  }


  auto printPaths = new QAction{"Print Paths to Highlighted Subtrees", this};

  connect(printPaths, &QAction::triggered, [this]() {
    m_Canvas->printHightlightedPaths();
  });

  QMenu* nodeMenu = menuBar->addMenu(tr("&Node"));
  nodeMenu->addAction(showNodeStats);


  bookmarksMenu->addAction(bookmarkNode);
  connect(bookmarksMenu, SIGNAL(aboutToShow()),
          this, SLOT(populateBookmarks()));

  nodeMenu->addMenu(bookmarksMenu);
  nodeMenu->addSeparator();
  nodeMenu->addAction(navUp);
  nodeMenu->addAction(navDown);
  nodeMenu->addAction(navLeft);
  nodeMenu->addAction(navRight);
  nodeMenu->addAction(navRoot);
  nodeMenu->addAction(navNextSol);
  nodeMenu->addAction(navPrevSol);
  nodeMenu->addAction(navNextLeaf);
  nodeMenu->addAction(navPrevLeaf);
  nodeMenu->addSeparator();
  nodeMenu->addAction(toggleHidden);
  nodeMenu->addAction(hideFailed);
  nodeMenu->addAction(unhideAll);
  nodeMenu->addAction(labelBranches);
  nodeMenu->addAction(labelPath);
  nodeMenu->addAction(showNogoods);
  nodeMenu->addAction(showNodeInfo);

  nodeMenu->addSeparator();
  nodeMenu->addAction(zoomToFit);
  nodeMenu->addAction(center);
  nodeMenu->addAction(exportPDF);
  nodeMenu->addSeparator();
  nodeMenu->addAction(findNode);

  /// ***** Tree Visualisaitons *****

  QMenu* treeVisMenu = menuBar->addMenu(tr("Tree"));

  treeVisMenu->addAction(highlightNodesMenu);
  treeVisMenu->addAction(analyzeSimilarSubtrees);
  treeVisMenu->addAction(showPixelTree);
  treeVisMenu->addAction(showIcicleTree);
  treeVisMenu->addAction(followPath);
  treeVisMenu->addAction(compareSubtreeLabels);
  treeVisMenu->addAction(printPaths);
  treeVisMenu->addAction(analyseBackjumps);
  treeVisMenu->addAction(deleteWhiteNodes);
  treeVisMenu->addAction(deleteTrials);
  treeVisMenu->addAction(deleteSkippedNodes);
  treeVisMenu->addAction(compareDomains);

#ifdef MAXIM_DEBUG
  treeVisMenu->addAction(createRandomTree);
#endif


  // Don't add the menu bar on Mac OS X
#ifndef Q_WS_MAC
  setMenuBar(menuBar);
#endif

  statusBar()->addPermanentWidget(m_NodeStatsBar.get());

  connect(this, SIGNAL(changeMainTitle(QString)),
          this, SLOT(changeTitle(QString)));

  connect(m_Canvas.get(), SIGNAL(contextMenu(QContextMenuEvent*)),
          this, SLOT(onContextMenu(QContextMenuEvent*)));

  connect(m_Canvas.get(), SIGNAL(addedBookmark(const QString&)), this, SLOT(addBookmark(const QString&)));
  connect(m_Canvas.get(), SIGNAL(removedBookmark(int)), this, SLOT(removeBookmark(int)));

  nullBookmark = new QAction("<none>",this);
  nullBookmark->setCheckable(true);
  nullBookmark->setChecked(false);
  nullBookmark->setEnabled(false);

  bookmarksGroup = new QActionGroup(this);
  bookmarksGroup->setExclusive(false);
  bookmarksGroup->addAction(nullBookmark);
  connect(bookmarksGroup, SIGNAL(triggered(QAction*)),
          this, SLOT(selectBookmark(QAction*)));

  nodeStatInspector = new NodeStatInspector(this, execution.nodeTree());

  preferences(true);
  show();
  reset->trigger();
}

void
GistMainWindow::updateStatsBar() {
  auto& stats = execution.nodeTree().getStatistics();
  m_NodeStatsBar->display(stats);
}

void
GistMainWindow::finishStatsBar() {
  QMutexLocker locker(&gistMutex);

  qDebug() << "finishStatsBar in thread: " << QThread::currentThreadId();

  qDebug() << "finish stats bar";

  /// a quick hack for now
  if (execution.getData().isDone()) {

    qDebug() << "~~~ Done building";

    auto totalTime = execution.getData().getTotalTime();
    float seconds = totalTime / 1000000.0;

    statusBar()->showMessage("Done in " + QString::number(seconds) + "s");
  } else {

    qDebug() << "~~~ not done building yet";
    statusBar()->showMessage("Searching");
  }

}

void
GistMainWindow::preferences(bool setup) {
  PreferencesDialog pd(this);
  if (setup) {
    setAutoZoom(pd.zoom);
  }
  if (setup || pd.exec() == QDialog::Accepted) {
    setAutoHideFailed(pd.hideFailed);
    setRefresh(pd.refresh);
    setRefreshPause(pd.refreshPause);
    setSmoothScrollAndZoom(pd.smoothScrollAndZoom);
    setMoveDuringSearch(pd.moveDuringSearch);
  }
}

void
GistMainWindow::populateBookmarks(void) {
  bookmarksMenu->clear();
  bookmarksMenu->addAction(bookmarkNode);
  bookmarksMenu->addSeparator();
  bookmarksMenu->addActions(bookmarksGroup->actions());
}

void
GistMainWindow::changeTitle(QString file_name) {
  qDebug() << "changing title to: " << file_name;
  this->setWindowTitle(file_name);
}

void
GistMainWindow::gatherStatistics(void) {
  std::ofstream out;
  out.open(statsFilename.toStdString(), std::ofstream::out);
  m_Canvas->collectMLStatsRoot(out);
  out.close();
}

void
GistMainWindow::selectNode(int gid) {
    m_Canvas->navigateToNodeById(gid);
}

void
GistMainWindow::selectManyNodes(QVariantList gids) {
    m_Canvas->unselectAll();
    for (int i = 0 ; i < gids.size() ; i++) {
        double d = gids[i].toDouble();
        int gid = (int) d;
        VisualNode* node = execution.nodeTree().getNode(gid);
        node->setSelected(true);
    }
    m_Canvas->updateCanvas();
}


void GistMainWindow::addActions() {

  auto canvas = m_Canvas.get();

  extractSubtree = new QAction("Extract Subtree", this);
  addAction(extractSubtree);
  connect(extractSubtree, &QAction::triggered, [this]() {

    auto nt_and_data = m_Canvas->extractSubtree();

    conductor.createExecution(std::move(nt_and_data.first),
                              std::move(nt_and_data.second));
  });

  findSelectedShape = new QAction("Find All of This Shape", this);
  addAction(findSelectedShape);
  connect(findSelectedShape, &QAction::triggered, [this]() {
    m_Canvas->findSelectedShape();
  });

  reset = new QAction("Reset", this);
  addAction(reset);
  connect(reset, SIGNAL(triggered()), canvas, SLOT(reset()));

  showPixelTree = new QAction("Pixel Tree View", this);
  addAction(showPixelTree);
  connect(showPixelTree, SIGNAL(triggered()), canvas, SLOT(showPixelTree()));

  showIcicleTree = new QAction("Icicle Tree View", this);
  addAction(showIcicleTree);
  connect(showIcicleTree, SIGNAL(triggered()), canvas, SLOT(showIcicleTree()));

  showWebscript = new QAction("Webscript View", this);
  addAction(showWebscript);
  /// NOTE(maxim): what is this (no slot)?
  // connect(showWebscript, SIGNAL(triggered()), canvas, SLOT(showWebscript()));

  followPath = new QAction("Follow Path", this);
  addAction(followPath);
  connect(followPath, SIGNAL(triggered()), canvas, SLOT(followPath()));

  compareSubtreeLabels = new QAction("Compare Subtree Labels", this);
  addAction(compareSubtreeLabels);
  connect(compareSubtreeLabels, &QAction::triggered, canvas, &TreeCanvas::compareSubtreeLabels);

    /// Expand current node
  auto expand_action = new QAction("Expand", this);
  expand_action->setShortcut(QKeySequence("Return"));
  addAction(expand_action);
  connect(expand_action, SIGNAL(triggered()), canvas, SLOT(expandCurrentNode()));

  /// Collect ML stats
  auto collectMLStats_action = new QAction("Collect ML stats", this);
  addAction(collectMLStats_action);
  connect(collectMLStats_action, SIGNAL(triggered()), canvas, SLOT(collectMLStats()));

  deleteWhiteNodes = new QAction{"Delete Unexplored Nodes", this};
  addAction(deleteWhiteNodes);
  connect(deleteWhiteNodes, &QAction::triggered, canvas, &TreeCanvas::deleteWhiteNodes);

  deleteTrials = new QAction{"Delete Trials", this};
  addAction(deleteTrials);
  connect(deleteTrials, &QAction::triggered, canvas, &TreeCanvas::deleteTrials);

  deleteSkippedNodes = new QAction{"Delete Skipped Nodes", this};
  addAction(deleteSkippedNodes);
  connect(deleteSkippedNodes, &QAction::triggered, canvas, &TreeCanvas::deleteSkippedNodes);

  compareDomains = new QAction{"Compare Domains", this};
  addAction(compareDomains);
  connect(compareDomains, &QAction::triggered, &execution, &Execution::compareDomains);

  navUp = new QAction("Up", this);
  addAction(navUp);
  navUp->setShortcut(QKeySequence("Up"));
  connect(navUp, SIGNAL(triggered()), canvas, SLOT(navUp()));

  navDown = new QAction("Down", this);
  addAction(navDown);
  navDown->setShortcut(QKeySequence("Down"));
  connect(navDown, SIGNAL(triggered()), canvas, SLOT(navDown()));

  navLeft = new QAction("Left", this);
  addAction(navLeft);
  navLeft->setShortcut(QKeySequence("Left"));
  connect(navLeft, SIGNAL(triggered()), canvas, SLOT(navLeft()));

  navRight = new QAction("Right", this);
  addAction(navRight);
  navRight->setShortcut(QKeySequence("Right"));
  connect(navRight, SIGNAL(triggered()), canvas, SLOT(navRight()));

  navRoot = new QAction("Root", this);
  addAction(navRoot);
  navRoot->setShortcut(QKeySequence("R"));
  connect(navRoot, SIGNAL(triggered()), canvas, SLOT(navRoot()));

  navNextSol = new QAction("To next solution", this);
  addAction(navNextSol);
  navNextSol->setShortcut(QKeySequence("Shift+Right"));
  connect(navNextSol, SIGNAL(triggered()), canvas, SLOT(navNextSol()));

  navPrevSol = new QAction("To previous solution", this);
  addAction(navPrevSol);
  navPrevSol->setShortcut(QKeySequence("Shift+Left"));
  connect(navPrevSol, SIGNAL(triggered()), canvas, SLOT(navPrevSol()));

  navNextLeaf = new QAction("To next leaf", this);
  addAction(navNextLeaf);
  navNextLeaf->setShortcut(QKeySequence("Ctrl+Right"));
  connect(navNextLeaf, SIGNAL(triggered()), canvas, SLOT(navNextLeaf()));

  navPrevLeaf = new QAction("To previous leaf", this);
  addAction(navPrevLeaf);
  navPrevLeaf->setShortcut(QKeySequence("Ctrl+Left"));
  connect(navPrevLeaf, SIGNAL(triggered()), canvas, SLOT(navPrevLeaf()));

  toggleHidden = new QAction("Hide/unhide", this);
  addAction(toggleHidden);
  toggleHidden->setShortcut(QKeySequence("H"));
  connect(toggleHidden, SIGNAL(triggered()), canvas, SLOT(toggleHidden()));

  hideFailed = new QAction("Hide failed subtrees", this);
  addAction(hideFailed);
  hideFailed->setShortcut(QKeySequence("F"));
  connect(hideFailed, SIGNAL(triggered()), canvas, SLOT(hideFailed()));

  analyseBackjumps = new QAction("Analyse Backjumps", this);
  addAction(analyseBackjumps);
  connect(analyseBackjumps, &QAction::triggered,
          canvas, &TreeCanvas::analyseBackjumps);

  findNode = new QAction("Find Node", this);
  addAction(findNode);
  findNode->setShortcut(QKeySequence("Ctrl+F"));
  connect(findNode, &QAction::triggered,
          canvas, &TreeCanvas::openNodeSearch);

  auto highlightSubtree = new QAction{"Toggle Highlight Subtree", this};
  connect(highlightSubtree, &QAction::triggered, canvas, &TreeCanvas::highlightSubtree);
  addAction(highlightSubtree);

#ifdef MAXIM_DEBUG
  auto printDebugInfo = new QAction("Print Debug Info", this);
  printDebugInfo->setShortcut(QKeySequence("Ctrl+Shift+D"));
  connect(printDebugInfo, &QAction::triggered, canvas,
          &TreeCanvas::printDebugInfo);
  addAction(printDebugInfo);

  auto updateCanvas = new QAction{"Update Canvas", this};
  updateCanvas->setShortcut(QKeySequence("Shift+U"));
  connect(updateCanvas, &QAction::triggered, [this, canvas]() {
      qDebug() << "action: update canvas";
      canvas->updateCanvas();
  });
  addAction(updateCanvas);

  auto addChildren = new QAction{"Add 2 Children", this};
  addChildren->setShortcut(QKeySequence("Shift+C"));
  connect(addChildren, &QAction::triggered, canvas, &TreeCanvas::addChildren);
  addAction(addChildren);

  auto nextStatus = new QAction{"Next Status", this};
  nextStatus->setShortcut(QKeySequence("Shift+V"));
  connect(nextStatus, &QAction::triggered, canvas, &TreeCanvas::nextStatus);
  addAction(nextStatus);

  auto deleteNode = new QAction{"Delete Node", this};
  deleteNode->setShortcut(QKeySequence("del"));
  connect(deleteNode, &QAction::triggered, canvas, &TreeCanvas::deleteSelectedNode);
  addAction(deleteNode);

  auto deleteMiddleNode = new QAction{"Delete Middle Node", this};
  deleteMiddleNode->setShortcut(QKeySequence("shift+del"));
  connect(deleteMiddleNode, &QAction::triggered, canvas, &TreeCanvas::deleteSelectedMiddleNode);
  addAction(deleteMiddleNode);

  createRandomTree = new QAction{"Create Random Tree", this};
  connect(createRandomTree, &QAction::triggered, canvas, &TreeCanvas::createRandomTree);
  addAction(createRandomTree);

  dirtyUpNode = new QAction{"Dirty Up Node", this};
  dirtyUpNode->setShortcut(QKeySequence("D"));
  connect(dirtyUpNode, &QAction::triggered, canvas, &TreeCanvas::dirtyUpNode);
  addAction(dirtyUpNode);

  auto computeShape = new QAction{"Compute shape", this};
  connect(computeShape, &QAction::triggered, canvas, &TreeCanvas::computeShape);

  auto setLabel = new QAction{"Set label", this};
  connect(setLabel, &QAction::triggered, canvas, &TreeCanvas::setLabel);


#endif

  unhideAll = new QAction("Unhide all", this);
  addAction(unhideAll);
  unhideAll->setShortcut(QKeySequence("U"));
  connect(unhideAll, SIGNAL(triggered()), canvas, SLOT(unhideAll()));

  labelBranches = new QAction("Label/clear branches", this);
  addAction(labelBranches);
  labelBranches->setShortcut(QKeySequence("L"));
  /// TODO(maxim): should this be the default (Qt::WindowShortcut) instead?
  labelBranches->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  connect(labelBranches, SIGNAL(triggered()), canvas, SLOT(labelBranches()));

  labelPath = new QAction("Label/clear path", this);
  addAction(labelPath);
  labelPath->setShortcut(QKeySequence("Shift+L"));
  connect(labelPath, SIGNAL(triggered()), canvas, SLOT(labelPath()));
  // labelPath->setShortcutContext(Qt::ApplicationShortcut);

  analyzeSimilarSubtrees = new QAction("Similar Subtree Analysis", this);
  addAction(analyzeSimilarSubtrees);
  analyzeSimilarSubtrees->setShortcut(QKeySequence("Shift+s"));
  connect(analyzeSimilarSubtrees, SIGNAL(triggered()), canvas, SLOT(analyzeSimilarSubtrees()));

  highlightNodesMenu = new QAction("Highlight Nodes Based On ...", this);
  addAction(highlightNodesMenu);
  connect(highlightNodesMenu, SIGNAL(triggered()), canvas, SLOT(highlightNodesMenu()));

  showNogoods = new QAction("Show no-goods", this);
  addAction(showNogoods);
  showNogoods->setShortcut(QKeySequence("Shift+N"));
  connect(showNogoods, SIGNAL(triggered()), canvas, SLOT(showNogoods()));

  showNodeInfo = new QAction("Show node info", this);
  addAction(showNodeInfo);
  showNodeInfo->setShortcut(QKeySequence("I"));
  connect(showNodeInfo, SIGNAL(triggered()), canvas, SLOT(showNodeInfo()));

  showNodeOnPixelTree = new QAction("Show node on a pixel tree", this);
  addAction(showNodeOnPixelTree);
  showNodeOnPixelTree->setShortcut(QKeySequence("J"));
  connect(showNodeOnPixelTree, SIGNAL(triggered()), canvas, SLOT(showNodeOnPixelTree()));

  zoomToFit = new QAction("Zoom to fit", this);
  addAction(zoomToFit);
  zoomToFit->setShortcut(QKeySequence("Z"));
  connect(zoomToFit, SIGNAL(triggered()), canvas, SLOT(zoomToFit()));

  center = new QAction("Center current node", this);
  addAction(center);
  center->setShortcut(QKeySequence("C"));
  connect(center, SIGNAL(triggered()), canvas, SLOT(centerCurrentNode()));

  exportPDF = new QAction("Export subtree PDF...", this);
  addAction(exportPDF);
  exportPDF->setShortcut(QKeySequence("P"));
  connect(exportPDF, SIGNAL(triggered()), canvas, SLOT(exportPDF()));

  exportWholeTreePDF = new QAction("Export PDF...", this);
  addAction(exportWholeTreePDF);
  exportWholeTreePDF->setShortcut(QKeySequence("Ctrl+Shift+P"));
  connect(exportWholeTreePDF, SIGNAL(triggered()), canvas, SLOT(exportWholeTreePDF()));

  print = new QAction("Print...", this);
  addAction(print);
  print->setShortcut(QKeySequence("Ctrl+P"));
  connect(print, SIGNAL(triggered()), canvas, SLOT(print()));

  bookmarkNode = new QAction("Add/remove bookmark", this);
  bookmarkNode->setShortcut(QKeySequence("Shift+B"));
  connect(bookmarkNode, SIGNAL(triggered()), canvas, SLOT(bookmarkNode()));

  showNodeStats = new QAction("Node statistics", this);
  addAction(showNodeStats);
  showNodeStats->setShortcut(QKeySequence("S"));
  connect(showNodeStats, &QAction::triggered, [this]() {
    nodeStatInspector->showStats(m_Canvas->getCurrentNode());
  });

  contextMenu = new QMenu(this);
  
  contextMenu->addAction(collectMLStats_action);
  contextMenu->addAction(showNodeStats);
  contextMenu->addAction(center);

  contextMenu->addSeparator();

  contextMenu->addAction(toggleHidden);
  contextMenu->addAction(hideFailed);
  contextMenu->addAction(unhideAll);
  contextMenu->addAction(labelBranches);
  contextMenu->addAction(labelPath);
  contextMenu->addAction(showNogoods);
  contextMenu->addAction(showNodeInfo);
  contextMenu->addAction(showNodeOnPixelTree);

  contextMenu->addSeparator();

  contextMenu->addMenu(bookmarksMenu);

  contextMenu->addSeparator();

  contextMenu->addAction(extractSubtree);
  contextMenu->addAction(findSelectedShape);
  contextMenu->addAction(highlightSubtree);

#ifdef MAXIM_DEBUG
  contextMenu->addAction(deleteNode);
  contextMenu->addAction(deleteMiddleNode);
  contextMenu->addAction(dirtyUpNode);
  contextMenu->addAction(computeShape);
  contextMenu->addAction(setLabel);
#endif

}

/// reacts on `TreeCanvas::currentNodeChanged`
void GistMainWindow::updateActions(VisualNode* n) {

  if (execution.getData().isDone()) {

    auto& na = execution.nodeTree().getNA();

    auto root = execution.nodeTree().getRoot();

    NextSolCursor nsc(n, false, na);

    PreorderNodeVisitor<NextSolCursor> nsv(nsc);
    nsv.run();
    navNextSol->setEnabled(nsv.getCursor().node() != root);

    NextSolCursor psc(n, true, na);

    PreorderNodeVisitor<NextSolCursor> psv(psc);
    psv.run();
    navPrevSol->setEnabled(psv.getCursor().node() != root);

  }
}
void
GistMainWindow::selectBookmark(QAction* a) {
    int idx = bookmarksGroup->actions().indexOf(a);
    m_Canvas->setCurrentNode(m_Canvas->bookmarks[idx]);
    m_Canvas->centerCurrentNode();
}

void
GistMainWindow::addBookmark(const QString& id) {
    if (bookmarksGroup->actions().indexOf(nullBookmark) != -1) {
        bookmarksGroup->removeAction(nullBookmark);
    }

    QAction* nb = new QAction(id, this);
    nb->setCheckable(true);
    bookmarksGroup->addAction(nb);
}

void
GistMainWindow::removeBookmark(int idx) {
    QAction* a = bookmarksGroup->actions()[idx];
    bookmarksGroup->removeAction(a);
    if (bookmarksGroup->actions().size() == 0) {
        bookmarksGroup->addAction(nullBookmark);
    }
}

void
GistMainWindow::populateBookmarksMenu(void) {
    bookmarksMenu->clear();
    bookmarksMenu->addAction(bookmarkNode);
    bookmarksMenu->addSeparator();
    bookmarksMenu->addActions(bookmarksGroup->actions());
}

void
GistMainWindow::onContextMenu(QContextMenuEvent* event) {
    contextMenu->popup(event->globalPos());
}

void
GistMainWindow::setAutoHideFailed(bool b) { m_Canvas->setAutoHideFailed(b); }
void
GistMainWindow::setAutoZoom(bool b) { m_Canvas->setAutoZoom(b); }
bool
GistMainWindow::getAutoHideFailed(void) { return m_Canvas->getAutoHideFailed(); }
bool
GistMainWindow::getAutoZoom(void) { return m_Canvas->getAutoZoom(); }
void
GistMainWindow::setRefresh(int i) { m_Canvas->setRefresh(i); }
void
GistMainWindow::setRefreshPause(int i) { m_Canvas->setRefreshPause(i); }
bool
GistMainWindow::getSmoothScrollAndZoom(void) {
    return m_Canvas->getSmoothScrollAndZoom();
}
void
GistMainWindow::setSmoothScrollAndZoom(bool b) {
    m_Canvas->setSmoothScrollAndZoom(b);
}
bool
GistMainWindow::getMoveDuringSearch(void) {
    return m_Canvas->getMoveDuringSearch();
}
void
GistMainWindow::setMoveDuringSearch(bool b) {
    m_Canvas->setMoveDuringSearch(b);
}

void
GistMainWindow::resizeEvent(QResizeEvent*) {
    m_Canvas->resizeToOuter();
}

void
GistMainWindow::emitChangeMainTitle(QString file_name) {
    emit changeMainTitle(file_name);
}

GistMainWindow::~GistMainWindow() = default;

TreeCanvas* GistMainWindow::getCanvas() { return m_Canvas.get(); }
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

#include <QAction>
#include <QGridLayout>

#include "qtgist.hh"

#include "nodestats.hh"
#include "nodevisitor.hh"
#include "nodecursor.hh"

#include "message.pb.hh"
#include "execution.hh"
#include "cmp_tree_dialog.hh"


Gist::Gist(Execution& e, QWidget* parent)
    : QWidget(parent), execution(e) {
  layout = new QGridLayout(this);

  auto scrollArea = new QAbstractScrollArea(this);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea->setAutoFillBackground(true);

  myPalette = new QPalette(scrollArea->palette());

  myPalette->setColor(QPalette::Window, Qt::white);
  scrollArea->setPalette(*myPalette);

  setLayout(layout);

  m_Canvas.reset(new TreeCanvas(&execution, layout, scrollArea->viewport()));
  m_Canvas->setPalette(*myPalette);
  m_Canvas->setObjectName("canvas");

  layout->addWidget(scrollArea, 0, 0, -1, 1);
  layout->addWidget(m_Canvas->scaleBar(), 1, 1, Qt::AlignHCenter);
  layout->addWidget(m_Canvas->smallBox, 1, 2, Qt::AlignBottom);

  addActions();

  connect(scrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
          m_Canvas.get(), SLOT(scroll(void)));
  connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)), m_Canvas.get(),
          SLOT(scroll(void)));

  connect(this, SIGNAL(doneReceiving()), m_Canvas.get(), SLOT(statusFinished()));

  connect(m_Canvas.get(), SIGNAL(addedBookmark(const QString&)), this, SLOT(addBookmark(const QString&)));
  connect(m_Canvas.get(), SIGNAL(removedBookmark(int)), this, SLOT(removeBookmark(int)));
  connect(m_Canvas.get(), SIGNAL(needActionsUpdate(VisualNode*, bool)),
          this, SLOT(updateActions(VisualNode*, bool)));

  connect(m_Canvas.get(), SIGNAL(statusChanged(VisualNode*, const Statistics&, bool)),
          this, SLOT(on_canvas_statusChanged(VisualNode*, const Statistics&, bool)));

  nodeStatInspector = new NodeStatInspector(this);

  m_Canvas->show();

  resize(500, 400);

  auto sa_layout = new QVBoxLayout();
  sa_layout->setContentsMargins(0, 0, 0, 0);
  sa_layout->addWidget(m_Canvas.get());

  scrollArea->viewport()->setLayout(sa_layout);

  // enables on_<sender>_<signal>() mechanism
  QMetaObject::connectSlotsByName(this);
}

void
Gist::resizeEvent(QResizeEvent*) {
    m_Canvas->resizeToOuter();
}

Gist::~Gist(void) = default;

void
Gist::on_canvas_contextMenu(QContextMenuEvent* event) {
    contextMenu->popup(event->globalPos());
}

void
Gist::updateActions(VisualNode* n, bool finished) {
    // qDebug() << "!! updateActions triggered";

    if (!finished) {
        navUp->setEnabled(false);
        navDown->setEnabled(false);
        navLeft->setEnabled(false);
        navRight->setEnabled(false);
        navRoot->setEnabled(false);
        unhideAll->setEnabled(false);
    } else {

        navRoot->setEnabled(true);

        if (n->getNumberOfChildren() > 0) {
            navDown->setEnabled(true);

        } else {
            navDown->setEnabled(false);
        }

        VisualNode* p = n->getParent(execution.getNA());

        if (p == nullptr) {
            navUp->setEnabled(false);
            navRight->setEnabled(false);
            navLeft->setEnabled(false);
        } else {
            navUp->setEnabled(true);

            unsigned int alt = n->getAlternative(execution.getNA());

            navRight->setEnabled(alt + 1 < p->getNumberOfChildren());
            navLeft->setEnabled(alt > 0);
        }

        if (n->getNumberOfChildren() > 0) {
            unhideAll->setEnabled(true);
        } else {
            unhideAll->setEnabled(false);
        }

    }
}


/// TODO
void
Gist::on_canvas_statusChanged(VisualNode* n, const Statistics& stats,
                              bool finished) {

    nodeStatInspector->node(execution.getNA(),n,stats,finished); /// for single node stats

    if (!finished) {
        showNodeStats->setEnabled(false);
        // stop-> setEnabled(true);
        // reset->setEnabled(false);

        navNextSol->setEnabled(false);
        navPrevSol->setEnabled(false);

        // searchNext->setEnabled(false);
        // searchAll->setEnabled(false);
        toggleHidden->setEnabled(false);
        hideFailed->setEnabled(false);
        // hideSize->setEnabled(false);

        // labelBranches->setEnabled(false);
        // labelPath->setEnabled(false);

        // toggleStop->setEnabled(false);
        // unstopAll->setEnabled(false);

        center->setEnabled(false); /// ??
        exportPDF->setEnabled(false);
        exportWholeTreePDF->setEnabled(false);
        print->setEnabled(false);
        // printSearchLog->setEnabled(false);

        bookmarkNode->setEnabled(false);
        bookmarksGroup->setEnabled(false);
    } else {
        // stop->setEnabled(false);
        // reset->setEnabled(true);

        if ( (n->isOpen() || n->hasOpenChildren()) && (!n->isHidden()) ) {
            // searchNext->setEnabled(true);
            // searchAll->setEnabled(true);
        } else {
            // searchNext->setEnabled(false);
            // searchAll->setEnabled(false);
        }
        if (n->getNumberOfChildren() > 0) {

            toggleHidden->setEnabled(true);
            hideFailed->setEnabled(true);
            // hideSize->setEnabled(true);
            // unstopAll->setEnabled(true);
        } else {
            toggleHidden->setEnabled(false);
            hideFailed->setEnabled(false);
            // hideSize->setEnabled(false);
            // unhideAll->setEnabled(false);
            // unstopAll->setEnabled(false);
        }

        toggleStop->setEnabled(n->getStatus() == STOP ||
                               n->getStatus() == UNSTOP);

        showNodeStats->setEnabled(true);
        labelPath->setEnabled(true);

        VisualNode* root = n;
        while (!root->isRoot()) {
            root = root->getParent(execution.getNA());
        }
        NextSolCursor nsc(n, false, execution.getNA());

        PreorderNodeVisitor<NextSolCursor> nsv(nsc);
        nsv.run();
        navNextSol->setEnabled(nsv.getCursor().node() != root);

        NextSolCursor psc(n, true, execution.getNA());

        PreorderNodeVisitor<NextSolCursor> psv(psc);
        psv.run();
        navPrevSol->setEnabled(psv.getCursor().node() != root);

        center->setEnabled(true);
        exportPDF->setEnabled(true);
        exportWholeTreePDF->setEnabled(true);
        print->setEnabled(true);
        printSearchLog->setEnabled(true);

        bookmarkNode->setEnabled(true);
        bookmarksGroup->setEnabled(true);
    }
    emit statusChanged(stats,finished);
}

void
Gist::emitChangeMainTitle(QString file_name) {
    emit changeMainTitle(file_name);
}

void
Gist::selectBookmark(QAction* a) {
    int idx = bookmarksGroup->actions().indexOf(a);
    m_Canvas->setCurrentNode(m_Canvas->bookmarks[idx]);
    m_Canvas->centerCurrentNode();
}

void
Gist::addBookmark(const QString& id) {
    if (bookmarksGroup->actions().indexOf(nullBookmark) != -1) {
        bookmarksGroup->removeAction(nullBookmark);
    }

    QAction* nb = new QAction(id, this);
    nb->setCheckable(true);
    bookmarksGroup->addAction(nb);
}

void
Gist::removeBookmark(int idx) {
    QAction* a = bookmarksGroup->actions()[idx];
    bookmarksGroup->removeAction(a);
    if (bookmarksGroup->actions().size() == 0) {
        bookmarksGroup->addAction(nullBookmark);
    }
}

void
Gist::populateBookmarksMenu(void) {
    bookmarksMenu->clear();
    bookmarksMenu->addAction(bookmarkNode);
    bookmarksMenu->addSeparator();
    bookmarksMenu->addActions(bookmarksGroup->actions());
}


void
Gist::setAutoHideFailed(bool b) { m_Canvas->setAutoHideFailed(b); }
void
Gist::setAutoZoom(bool b) { m_Canvas->setAutoZoom(b); }
bool
Gist::getAutoHideFailed(void) { return m_Canvas->getAutoHideFailed(); }
bool
Gist::getAutoZoom(void) { return m_Canvas->getAutoZoom(); }
void
Gist::setRefresh(int i) { m_Canvas->setRefresh(i); }
void
Gist::setRefreshPause(int i) { m_Canvas->setRefreshPause(i); }
bool
Gist::getSmoothScrollAndZoom(void) {
    return m_Canvas->getSmoothScrollAndZoom();
}
void
Gist::setSmoothScrollAndZoom(bool b) {
    m_Canvas->setSmoothScrollAndZoom(b);
}
bool
Gist::getMoveDuringSearch(void) {
    return m_Canvas->getMoveDuringSearch();
}
void
Gist::setMoveDuringSearch(bool b) {
    m_Canvas->setMoveDuringSearch(b);
}
void
Gist::showStats(void) {
    nodeStatInspector->showStats();
    m_Canvas->emitStatusChanged();
}

void
Gist::addActions() {

    /// Expand current node
    auto expand_action = new QAction("Expand", this);
    expand_action->setShortcut(QKeySequence("Return"));
    addAction(expand_action);
    connect(expand_action, SIGNAL(triggered()), m_Canvas.get(), SLOT(expandCurrentNode()));

    reset = new QAction("Reset", this);
    addAction(reset);
    connect(reset, SIGNAL(triggered()), m_Canvas.get(), SLOT(reset()));
    // reset->setShortcut(QKeySequence("Ctrl+R"));

    showPixelTree = new QAction("Pixel Tree View", this);
    addAction(showPixelTree);
    connect(showPixelTree, SIGNAL(triggered()), m_Canvas.get(), SLOT(showPixelTree()));

    showIcicleTree = new QAction("Icicle Tree View", this);
    addAction(showIcicleTree);
    connect(showIcicleTree, SIGNAL(triggered()), m_Canvas.get(), SLOT(showIcicleTree()));

    showWebscript = new QAction("Webscript View", this);
    addAction(showWebscript);
    connect(showWebscript, SIGNAL(triggered()), m_Canvas.get(), SLOT(showWebscript()));

    followPath = new QAction("Follow Path", this);
    addAction(followPath);
    connect(followPath, SIGNAL(triggered()), m_Canvas.get(), SLOT(followPath()));

    /// Collect ML stats
    auto collectMLStats_action = new QAction("Collect ML stats", this);
    addAction(collectMLStats_action);
    connect(collectMLStats_action, SIGNAL(triggered()), m_Canvas.get(), SLOT(collectMLStats()));

    deleteWhiteNodes = new QAction{"Delete Unexplored Nodes", this};
    addAction(deleteWhiteNodes);
    connect(deleteWhiteNodes, &QAction::triggered, m_Canvas.get(), &TreeCanvas::deleteWhiteNodes);

    navUp = new QAction("Up", this);
    addAction(navUp);
    navUp->setShortcut(QKeySequence("Up"));
    connect(navUp, SIGNAL(triggered()), m_Canvas.get(), SLOT(navUp()));

    navDown = new QAction("Down", this);
    addAction(navDown);
    navDown->setShortcut(QKeySequence("Down"));
    connect(navDown, SIGNAL(triggered()), m_Canvas.get(), SLOT(navDown()));

    navLeft = new QAction("Left", this);
    addAction(navLeft);
    navLeft->setShortcut(QKeySequence("Left"));
    connect(navLeft, SIGNAL(triggered()), m_Canvas.get(), SLOT(navLeft()));

    navRight = new QAction("Right", this);
    addAction(navRight);
    navRight->setShortcut(QKeySequence("Right"));
    connect(navRight, SIGNAL(triggered()), m_Canvas.get(), SLOT(navRight()));

    navRoot = new QAction("Root", this);
    addAction(navRoot);
    navRoot->setShortcut(QKeySequence("R"));
    connect(navRoot, SIGNAL(triggered()), m_Canvas.get(), SLOT(navRoot()));

    navNextSol = new QAction("To next solution", this);
    addAction(navNextSol);
    navNextSol->setShortcut(QKeySequence("Shift+Right"));
    connect(navNextSol, SIGNAL(triggered()), m_Canvas.get(), SLOT(navNextSol()));

    navPrevSol = new QAction("To previous solution", this);
    addAction(navPrevSol);
    navPrevSol->setShortcut(QKeySequence("Shift+Left"));
    connect(navPrevSol, SIGNAL(triggered()), m_Canvas.get(), SLOT(navPrevSol()));

    navNextLeaf = new QAction("To next leaf", this);
    addAction(navNextLeaf);
    navNextLeaf->setShortcut(QKeySequence("Ctrl+Right"));
    connect(navNextLeaf, SIGNAL(triggered()), m_Canvas.get(), SLOT(navNextLeaf()));

    navPrevLeaf = new QAction("To previous leaf", this);
    addAction(navPrevLeaf);
    navPrevLeaf->setShortcut(QKeySequence("Ctrl+Left"));
    connect(navPrevLeaf, SIGNAL(triggered()), m_Canvas.get(), SLOT(navPrevLeaf()));

    searchNext = new QAction("Next solution", this);
    addAction(searchNext);
    searchNext->setShortcut(QKeySequence("N"));

    searchAll = new QAction("All solutions", this);
    addAction(searchAll);
    searchAll->setShortcut(QKeySequence("A"));

    toggleHidden = new QAction("Hide/unhide", this);
    addAction(toggleHidden);
    toggleHidden->setShortcut(QKeySequence("H"));
    connect(toggleHidden, SIGNAL(triggered()), m_Canvas.get(), SLOT(toggleHidden()));

    hideFailed = new QAction("Hide failed subtrees", this);
    addAction(hideFailed);
    hideFailed->setShortcut(QKeySequence("F"));
    connect(hideFailed, SIGNAL(triggered()), m_Canvas.get(), SLOT(hideFailed()));

    hideSize = new QAction("Hide small subtrees", this);
    addAction(hideSize);
    connect(hideSize, SIGNAL(triggered()), m_Canvas.get(), SLOT(hideSize()));

#ifdef MAXIM_DEBUG
    auto printDebugInfo = new QAction("Print Debug Info", this);
    printDebugInfo->setShortcut(QKeySequence("Ctrl+Shift+D"));
    connect(printDebugInfo, &QAction::triggered, m_Canvas.get(),
            &TreeCanvas::printDebugInfo);
    addAction(printDebugInfo);

    auto updateCanvas = new QAction{"Update Canvas", this};
    updateCanvas->setShortcut(QKeySequence("Shift+U"));
    connect(updateCanvas, &QAction::triggered, [this]() {
        qDebug() << "action: update canvas";
        m_Canvas->update();
    });
    addAction(updateCanvas);

    auto addChildren = new QAction{"Add 2 Children", this};
    addChildren->setShortcut(QKeySequence("Shift+C"));
    connect(addChildren, &QAction::triggered, m_Canvas.get(), &TreeCanvas::addChildren);
    addAction(addChildren);

    deleteNode = new QAction{"Delete Node", this};
    deleteNode->setShortcut(QKeySequence("del"));
    connect(deleteNode, &QAction::triggered, m_Canvas.get(), &TreeCanvas::deleteSelectedNode);
    addAction(deleteNode);

    dirtyUpNode = new QAction{"Dirty Up Node", this};
    dirtyUpNode->setShortcut(QKeySequence("D"));
    connect(dirtyUpNode, &QAction::triggered, m_Canvas.get(), &TreeCanvas::dirtyUpNode);
    addAction(dirtyUpNode);
#endif

    unhideAll = new QAction("Unhide all", this);
    addAction(unhideAll);
    unhideAll->setShortcut(QKeySequence("U"));
    connect(unhideAll, SIGNAL(triggered()), m_Canvas.get(), SLOT(unhideAll()));

    labelBranches = new QAction("Label/clear branches", this);
    addAction(labelBranches);
    labelBranches->setShortcut(QKeySequence("L"));
    /// TODO(maxim): should this be the default (Qt::WindowShortcut) instead?
    labelBranches->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(labelBranches, SIGNAL(triggered()), m_Canvas.get(), SLOT(labelBranches()));

    labelPath = new QAction("Label/clear path", this);
    addAction(labelPath);
    labelPath->setShortcut(QKeySequence("Shift+L"));
    connect(labelPath, SIGNAL(triggered()), m_Canvas.get(), SLOT(labelPath()));
    // labelPath->setShortcutContext(Qt::ApplicationShortcut);

    analyzeSimilarSubtrees = new QAction("Symilar Subtree Analysis", this);
    addAction(analyzeSimilarSubtrees);
    analyzeSimilarSubtrees->setShortcut(QKeySequence("Shift+s"));
    connect(analyzeSimilarSubtrees, SIGNAL(triggered()), m_Canvas.get(), SLOT(analyzeSimilarSubtrees()));

    highlightNodesMenu = new QAction("Highlight Nodes Based On ...", this);
    addAction(highlightNodesMenu);
    connect(highlightNodesMenu, SIGNAL(triggered()), m_Canvas.get(), SLOT(highlightNodesMenu()));

    showNogoods = new QAction("Show no-goods", this);
    addAction(showNogoods);
    showNogoods->setShortcut(QKeySequence("Shift+N"));
    connect(showNogoods, SIGNAL(triggered()), m_Canvas.get(), SLOT(showNogoods()));

    showNodeInfo = new QAction("Show node info", this);
    addAction(showNodeInfo);
    showNodeInfo->setShortcut(QKeySequence("I"));
    connect(showNodeInfo, SIGNAL(triggered()), m_Canvas.get(), SLOT(showNodeInfo()));

    showNodeOnPixelTree = new QAction("Show node on a pixel tree", this);
    addAction(showNodeOnPixelTree);
    showNodeOnPixelTree->setShortcut(QKeySequence("J"));
    connect(showNodeOnPixelTree, SIGNAL(triggered()), m_Canvas.get(), SLOT(showNodeOnPixelTree()));

    toggleStop = new QAction("Stop/unstop", this);
    addAction(toggleStop);
    toggleStop->setShortcut(QKeySequence("X"));
    connect(toggleStop, SIGNAL(triggered()), m_Canvas.get(), SLOT(toggleStop()));

    unstopAll = new QAction("Do not stop in subtree", this);
    addAction(unstopAll);
    unstopAll->setShortcut(QKeySequence("Shift+X"));
    connect(unstopAll, SIGNAL(triggered()), m_Canvas.get(), SLOT(unstopAll()));

    zoomToFit = new QAction("Zoom to fit", this);
    addAction(zoomToFit);
    zoomToFit->setShortcut(QKeySequence("Z"));
    connect(zoomToFit, SIGNAL(triggered()), m_Canvas.get(), SLOT(zoomToFit()));

    center = new QAction("Center current node", this);
    addAction(center);
    center->setShortcut(QKeySequence("C"));
    connect(center, SIGNAL(triggered()), m_Canvas.get(), SLOT(centerCurrentNode()));

    exportPDF = new QAction("Export subtree PDF...", this);
    addAction(exportPDF);
    exportPDF->setShortcut(QKeySequence("P"));
    connect(exportPDF, SIGNAL(triggered()), m_Canvas.get(), SLOT(exportPDF()));

    exportWholeTreePDF = new QAction("Export PDF...", this);
    addAction(exportWholeTreePDF);
    exportWholeTreePDF->setShortcut(QKeySequence("Ctrl+Shift+P"));
    connect(exportWholeTreePDF, SIGNAL(triggered()), m_Canvas.get(), SLOT(exportWholeTreePDF()));

    print = new QAction("Print...", this);
    addAction(print);
    print->setShortcut(QKeySequence("Ctrl+P"));
    connect(print, SIGNAL(triggered()), m_Canvas.get(), SLOT(print()));

    printSearchLog = new QAction("Export search log...", this);
    addAction(printSearchLog);
    connect(printSearchLog, SIGNAL(triggered()), m_Canvas.get(), SLOT(printSearchLog()));

    bookmarkNode = new QAction("Add/remove bookmark", this);
    bookmarkNode->setShortcut(QKeySequence("Shift+B"));
    connect(bookmarkNode, SIGNAL(triggered()), m_Canvas.get(), SLOT(bookmarkNode()));

    nullBookmark = new QAction("<none>",this);
    nullBookmark->setCheckable(true);
    nullBookmark->setChecked(false);
    nullBookmark->setEnabled(false);

    bookmarksGroup = new QActionGroup(this);
    bookmarksGroup->setExclusive(false);
    bookmarksGroup->addAction(nullBookmark);
    connect(bookmarksGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(selectBookmark(QAction*)));

    bookmarksMenu = new QMenu("Bookmarks", this);
    connect(bookmarksMenu, SIGNAL(aboutToShow()),
            this, SLOT(populateBookmarksMenu()));

    showNodeStats = new QAction("Node statistics", this);
    addAction(showNodeStats);
    showNodeStats->setShortcut(QKeySequence("S"));
    connect(showNodeStats, SIGNAL(triggered()),
            this, SLOT(showStats()));

    nullComparator = new QAction("<none>",this);
    nullComparator->setCheckable(true);
    nullComparator->setChecked(false);
    nullComparator->setEnabled(false);
    comparatorGroup = new QActionGroup(this);
    comparatorGroup->setExclusive(false);
    comparatorGroup->addAction(nullComparator);
//    connect(comparatorGroup, SIGNAL(triggered(QAction*)),
//            this, SLOT(selectComparator(QAction*)));

    comparatorMenu = new QMenu("Comparators", this);
    comparatorMenu->addActions(comparatorGroup->actions());

    contextMenu = new QMenu(this);
    
    contextMenu->addAction(collectMLStats_action);
    contextMenu->addAction(showNodeStats);
    contextMenu->addAction(center);

    contextMenu->addSeparator();

    contextMenu->addAction(searchNext);
    contextMenu->addAction(searchAll);

    contextMenu->addSeparator();

    contextMenu->addAction(toggleHidden);
    contextMenu->addAction(hideFailed);
    contextMenu->addAction(hideSize);
    contextMenu->addAction(unhideAll);
    contextMenu->addAction(labelBranches);
    contextMenu->addAction(labelPath);
    contextMenu->addAction(showNogoods);
    contextMenu->addAction(showNodeInfo);
    contextMenu->addAction(showNodeOnPixelTree);
    
    contextMenu->addAction(toggleStop);
    contextMenu->addAction(unstopAll);

    contextMenu->addSeparator();

    contextMenu->addMenu(bookmarksMenu);

    contextMenu->addSeparator();

#ifdef MAXIM_DEBUG
    contextMenu->addAction(deleteNode);
    contextMenu->addAction(dirtyUpNode);
#endif

}


TreeCanvas* Gist::getCanvas() { return m_Canvas.get(); }
Execution* Gist::getExecution() { return &execution; }
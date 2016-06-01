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

#include "qtgist.hh"

#include "nodevisitor.hh"
#include "nodecursor.hh"

#include "solver_tree_dialog.hh"

#include "message.pb.hh"
#include "execution.hh"
#include "cmp_tree_dialog.hh"


void
Gist::initInterface(void) {

    layout = new QGridLayout(this);

    scrollArea = new QAbstractScrollArea(this);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setAutoFillBackground(true);

    myPalette = new QPalette(scrollArea->palette());

    myPalette->setColor(QPalette::Window, Qt::white);
    scrollArea->setPalette(*myPalette);

    setLayout(layout);
}

Gist::Gist(Execution* execution, QWidget* parent) : QWidget(parent), execution(execution) {

    qDebug() << "--------------- Created Gist!";

    initInterface();
    addActions();

    canvas = new TreeCanvas(execution, layout, CanvasType::REGULAR, scrollArea->viewport());
    canvas->setPalette(*myPalette);
    canvas->setObjectName("canvas");

    layout->addWidget(scrollArea, 0,0,-1,1);
    layout->addWidget(canvas->scaleBar, 1,1, Qt::AlignHCenter);
    layout->addWidget(canvas->smallBox, 1,2, Qt::AlignBottom);

    connectCanvas(canvas);

    connect(scrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            canvas, SLOT(scroll(void)));
    connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
            canvas, SLOT(scroll(void)));

    // connect(canvas, SIGNAL(needActionsUpdate(VisualNode*, bool)),
    //         this, SLOT(updateActions(VisualNode*, bool)));


    // create new TreeCanvas when receiver gets new data
    // connect(receiver, SIGNAL(newCanvasNeeded()), this, SLOT(createNewCanvas(void)),
    //    Qt::BlockingQueuedConnection);

    // connect(canvas, SIGNAL(buildingFinished()), this, SIGNAL(buildingFinished()));


    nodeStatInspector = new NodeStatInspector(this);

    // receiver->receive(canvas);
    canvas->show();

    resize(500, 400);

    QVBoxLayout* sa_layout = new QVBoxLayout();
    sa_layout->setContentsMargins(0,0,0,0);
    sa_layout->addWidget(canvas);

    scrollArea->viewport()->setLayout(sa_layout);


    // enables on_<sender>_<signal>() mechanism
    QMetaObject::connectSlotsByName(this);
}

void
Gist::resizeEvent(QResizeEvent*) {
    canvas->resizeToOuter();
}

TreeCanvas*
Gist::getLastCanvas(void) {
    if (_td_vec.size() > 0)
        return _td_vec.back()->getCanvas();
    return nullptr;
}

SolverTreeDialog*
Gist::getLastTreeDialog(void) {
    if (_td_vec.size() > 0)
        return _td_vec.back();
    return nullptr;
}


Gist::~Gist(void) {

    qDebug() << "in Gist destructor";

    // receiver->terminate();
    // receiver->wait();

    for (auto &td : _td_vec) {
        delete td;
    }

    delete canvas;

    delete myPalette;
}

void
Gist::prepareNewCanvas(void) {
    canvasTwo->show();

}

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

        VisualNode* p = n->getParent(execution->getNA());
        if (p == nullptr) {
            navUp->setEnabled(false);
            navRight->setEnabled(false);
            navLeft->setEnabled(false);
        } else {
            navUp->setEnabled(true);
            unsigned int alt = n->getAlternative(execution->getNA());
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
    nodeStatInspector->node(execution->getNA(),n,stats,finished); /// for single node stats
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
        while (!root->isRoot())
            root = root->getParent(execution->getNA());
        NextSolCursor nsc(n, false, execution->getNA());
        PreorderNodeVisitor<NextSolCursor> nsv(nsc);
        nsv.run();
        navNextSol->setEnabled(nsv.getCursor().node() != root);

        NextSolCursor psc(n, true, execution->getNA());
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

bool
Gist::finish(void) {
    return canvas->finish();
}

//void
//Gist::selectComparator(QAction* a) {
//    canvas->activateComparator(comparatorGroup->actions().indexOf(a),
//                               a->isChecked());
//}
void
Gist::selectBookmark(QAction* a) {
    int idx = bookmarksGroup->actions().indexOf(a);
    canvas->setCurrentNode(canvas->bookmarks[idx]);
    canvas->centerCurrentNode();
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
Gist::setAutoHideFailed(bool b) { canvas->setAutoHideFailed(b); }
void
Gist::setAutoZoom(bool b) { canvas->setAutoZoom(b); }
bool
Gist::getAutoHideFailed(void) { return canvas->getAutoHideFailed(); }
bool
Gist::getAutoZoom(void) { return canvas->getAutoZoom(); }
void
Gist::setRefresh(int i) { canvas->setRefresh(i); }
void
Gist::setRefreshPause(int i) { canvas->setRefreshPause(i); }
bool
Gist::getSmoothScrollAndZoom(void) {
    return canvas->getSmoothScrollAndZoom();
}
void
Gist::setSmoothScrollAndZoom(bool b) {
    canvas->setSmoothScrollAndZoom(b);
}
bool
Gist::getMoveDuringSearch(void) {
    return canvas->getMoveDuringSearch();
}
void
Gist::setMoveDuringSearch(bool b) {
    canvas->setMoveDuringSearch(b);
}
void
Gist::showStats(void) {
    nodeStatInspector->showStats();
    canvas->emitStatusChanged();
}



void
Gist::addActions(void) {
    printDebugInfo = new QAction("Print Debug Info", this);
    printDebugInfo->setShortcut(QKeySequence("Ctrl+Shift+D"));

    expand = new QAction("Expand", this);
    expand->setShortcut(QKeySequence("Return"));

    stop = new QAction("Stop search", this);
    stop->setShortcut(QKeySequence("Esc"));

    reset = new QAction("Reset", this);
    reset->setShortcut(QKeySequence("Ctrl+R"));

    showPixelTree = new QAction("Pixel Tree View", this);
    showIcicleTree = new QAction("Icicle Tree View", this);
    showWebscript = new QAction("Webscript View", this);
    followPath = new QAction("Follow Path", this);

    navUp = new QAction("Up", this);
    navUp->setShortcut(QKeySequence("Up"));

    navDown = new QAction("Down", this);
    navDown->setShortcut(QKeySequence("Down"));

    navLeft = new QAction("Left", this);
    navLeft->setShortcut(QKeySequence("Left"));

    navRight = new QAction("Right", this);
    navRight->setShortcut(QKeySequence("Right"));

    navRoot = new QAction("Root", this);
    navRoot->setShortcut(QKeySequence("R"));

    navNextSol = new QAction("To next solution", this);
    navNextSol->setShortcut(QKeySequence("Shift+Right"));

    navPrevSol = new QAction("To previous solution", this);
    navPrevSol->setShortcut(QKeySequence("Shift+Left"));

    searchNext = new QAction("Next solution", this);
    searchNext->setShortcut(QKeySequence("N"));

    searchAll = new QAction("All solutions", this);
    searchAll->setShortcut(QKeySequence("A"));

    toggleHidden = new QAction("Hide/unhide", this);
    toggleHidden->setShortcut(QKeySequence("H"));

    hideFailed = new QAction("Hide failed subtrees", this);
    hideFailed->setShortcut(QKeySequence("F"));

    hideSize = new QAction("Hide small subtrees", this);

    unhideAll = new QAction("Unhide all", this);
    unhideAll->setShortcut(QKeySequence("U"));

    labelBranches = new QAction("Label/clear branches", this);
    labelBranches->setShortcut(QKeySequence("L"));
    /// TODO(maxim): should this be the default (Qt::WindowShortcut) instead?
    labelBranches->setShortcutContext(Qt::WidgetWithChildrenShortcut);

    labelPath = new QAction("Label/clear path", this);
    labelPath->setShortcut(QKeySequence("Shift+L"));
    // labelPath->setShortcutContext(Qt::ApplicationShortcut);

    analyzeSimilarSubtrees = new QAction("Symilar Subtree Analysis", this);
    analyzeSimilarSubtrees->setShortcut(QKeySequence("Shift+s"));

    highlightNodesMenu = new QAction("Highlight Nodes Based On ...", this);

    showNogoods = new QAction("Show no-goods", this);
    showNogoods->setShortcut(QKeySequence("Shift+N"));

    showNodeInfo = new QAction("Show node info", this);
    showNodeInfo->setShortcut(QKeySequence("I"));

    showNodeOnPixelTree = new QAction("Show node on a pixel tree", this);
    showNodeOnPixelTree->setShortcut(QKeySequence("J"));

    collectMLStats = new QAction("Collect ML stats", this);

    toggleStop = new QAction("Stop/unstop", this);
    toggleStop->setShortcut(QKeySequence("X"));

    unstopAll = new QAction("Do not stop in subtree", this);
    unstopAll->setShortcut(QKeySequence("Shift+X"));

    zoomToFit = new QAction("Zoom to fit", this);
    zoomToFit->setShortcut(QKeySequence("Z"));

    center = new QAction("Center current node", this);
    center->setShortcut(QKeySequence("C"));

    exportPDF = new QAction("Export subtree PDF...", this);
    exportPDF->setShortcut(QKeySequence("P"));

    exportWholeTreePDF = new QAction("Export PDF...", this);
    exportWholeTreePDF->setShortcut(QKeySequence("Ctrl+Shift+P"));

    print = new QAction("Print...", this);
    print->setShortcut(QKeySequence("Ctrl+P"));

    printSearchLog = new QAction("Export search log...", this);

    bookmarkNode = new QAction("Add/remove bookmark", this);
    bookmarkNode->setShortcut(QKeySequence("Shift+B"));

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

    addAction(printDebugInfo);

    addAction(expand);
    addAction(stop);
    addAction(reset);
    addAction(navUp);
    addAction(navDown);
    addAction(navLeft);
    addAction(navRight);
    addAction(navRoot);
    addAction(navNextSol);
    addAction(navPrevSol);

    addAction(searchNext);
    addAction(searchAll);
    addAction(toggleHidden);
    addAction(hideFailed);
    addAction(hideSize);
    addAction(unhideAll);
    addAction(labelBranches);
    addAction(labelPath);
    addAction(showPixelTree);
    addAction(showIcicleTree);
    addAction(showWebscript);
    addAction(followPath);
    addAction(analyzeSimilarSubtrees);
    addAction(highlightNodesMenu);
    addAction(showNogoods);
    addAction(showNodeInfo);
    addAction(showNodeOnPixelTree);
    addAction(collectMLStats);
    addAction(toggleStop);
    addAction(unstopAll);
    addAction(zoomToFit);
    addAction(center);
    addAction(exportPDF);
    addAction(exportWholeTreePDF);
    addAction(print);
    addAction(printSearchLog);

    addAction(showNodeStats);

    contextMenu = new QMenu(this);
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
    contextMenu->addAction(collectMLStats);

    contextMenu->addAction(toggleStop);
    contextMenu->addAction(unstopAll);

    contextMenu->addSeparator();

    contextMenu->addMenu(bookmarksMenu);

    contextMenu->addSeparator();
}

void
Gist::connectCanvas(TreeCanvas* tc) {

    qDebug() << "--- connectCanvas\n";


    connect(this, SIGNAL(doneReceiving()), tc, SLOT(statusFinished()));

    if (current_tc == tc) return;

    // if (current_tc && current_tc->_builder) {

    //     qDebug() << "--- disconnecting stuff\n";

    //     abort();
    // }

    current_tc = tc;

    connect(printDebugInfo, &QAction::triggered, tc, &TreeCanvas::printDebugInfo);

    /// TODO: these 2 should not be here

    connect(expand, SIGNAL(triggered()), tc, SLOT(expandCurrentNode()));
    connect(stop, SIGNAL(triggered()), tc, SLOT(stopSearch()));
    connect(reset, SIGNAL(triggered()), tc, SLOT(reset()));
    connect(navUp, SIGNAL(triggered()), tc, SLOT(navUp()));
    connect(navDown, SIGNAL(triggered()), tc, SLOT(navDown()));
    connect(navLeft, SIGNAL(triggered()), tc, SLOT(navLeft()));
    connect(navRight, SIGNAL(triggered()), tc, SLOT(navRight()));
    connect(navRoot, SIGNAL(triggered()), tc, SLOT(navRoot()));
    connect(navNextSol, SIGNAL(triggered()), tc, SLOT(navNextSol()));
    connect(navPrevSol, SIGNAL(triggered()), tc, SLOT(navPrevSol()));
    connect(toggleHidden, SIGNAL(triggered()), tc, SLOT(toggleHidden()));
    connect(hideFailed, SIGNAL(triggered()), tc, SLOT(hideFailed()));
    connect(hideSize, SIGNAL(triggered()), tc, SLOT(hideSize()));
    connect(labelBranches, SIGNAL(triggered()), tc, SLOT(labelBranches()));
    connect(unhideAll, SIGNAL(triggered()), tc, SLOT(unhideAll()));
    connect(labelPath, SIGNAL(triggered()), tc, SLOT(labelPath()));
    connect(showPixelTree, SIGNAL(triggered()), tc, SLOT(showPixelTree()));
    connect(showIcicleTree, SIGNAL(triggered()), tc, SLOT(showIcicleTree()));
    connect(showWebscript, SIGNAL(triggered()), tc, SLOT(showWebscript()));
    connect(followPath, SIGNAL(triggered()), tc, SLOT(followPath()));
    connect(analyzeSimilarSubtrees, SIGNAL(triggered()), tc, SLOT(analyzeSimilarSubtrees()));
    connect(highlightNodesMenu, SIGNAL(triggered()), tc, SLOT(highlightNodesMenu()));
    connect(showNogoods, SIGNAL(triggered()), current_tc, SLOT(showNogoods()));
    connect(showNodeInfo, SIGNAL(triggered()), current_tc, SLOT(showNodeInfo()));
    connect(showNodeOnPixelTree, SIGNAL(triggered()), current_tc, SLOT(showNodeOnPixelTree()));
    connect(collectMLStats, SIGNAL(triggered()), current_tc, SLOT(collectMLStats()));
    connect(toggleStop, SIGNAL(triggered()), tc, SLOT(toggleStop()));
    connect(unstopAll, SIGNAL(triggered()), tc, SLOT(unstopAll()));
    connect(zoomToFit, SIGNAL(triggered()), tc, SLOT(zoomToFit()));
    connect(center, SIGNAL(triggered()), tc, SLOT(centerCurrentNode()));
    connect(exportWholeTreePDF, SIGNAL(triggered()), tc, SLOT(exportWholeTreePDF()));
    connect(exportPDF, SIGNAL(triggered()), tc, SLOT(exportPDF()));
    connect(print, SIGNAL(triggered()), tc, SLOT(print()));
    connect(printSearchLog, SIGNAL(triggered()), tc, SLOT(printSearchLog()));
    connect(bookmarkNode, SIGNAL(triggered()), tc, SLOT(bookmarkNode()));
    connect(tc, SIGNAL(addedBookmark(const QString&)), this, SLOT(addBookmark(const QString&)));
    connect(tc, SIGNAL(removedBookmark(int)), this, SLOT(removeBookmark(int)));
    connect(tc, SIGNAL(needActionsUpdate(VisualNode*, bool)),
            this, SLOT(updateActions(VisualNode*, bool)));

}

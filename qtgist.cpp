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
#include "cmp_tree_dialog.hh"


void
Gist::createNewCanvas(void) {

    qDebug() << "!!! about to create a new canvas";

    SolverTreeDialog* td = new SolverTreeDialog(receiver, CanvasType::REGULAR, this);
    _td_vec.push_back(td);

}

void 
Gist::initiateComparison(void) {

    if (_td_vec.size() == 0) return;

    /// will be destroyed with Gist
    new CmpTreeDialog(receiver, CanvasType::MERGED, this,
                      canvas, _td_vec[0]->getCanvas());
}

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

Gist::Gist(QWidget* parent) : QWidget(parent) {

    initInterface();
    addActions();

    receiver = new ReceiverThread(this);

    canvas = new TreeCanvas(layout, receiver, CanvasType::REGULAR, scrollArea->viewport());
    canvas->setPalette(*myPalette);
    canvas->setObjectName("canvas");

    layout->addWidget(scrollArea, 0,0,-1,1);
    layout->addWidget(canvas->scaleBar, 1,1, Qt::AlignHCenter);
    layout->addWidget(canvas->smallBox, 1,2, Qt::AlignBottom);
    
    connectCanvas(canvas);

    connect(initComparison, SIGNAL(triggered()), this, SLOT(initiateComparison()));

    connect(scrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            canvas, SLOT(scroll(void)));
    connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
            canvas, SLOT(scroll(void)));

    // connect(canvas, SIGNAL(needActionsUpdate(VisualNode*, bool)),
    //         this, SLOT(updateActions(VisualNode*, bool)));


    // create new TreeCanvas when receiver gets new data
    connect(receiver, SIGNAL(newCanvasNeeded()), this, SLOT(createNewCanvas(void)),
       Qt::BlockingQueuedConnection);


    nodeStatInspector = new NodeStatInspector(this);

    receiver->receive(canvas);
    canvas->show();

    resize(500, 400);

    QVBoxLayout* sa_layout = new QVBoxLayout();
    sa_layout->setContentsMargins(0,0,0,0);
    sa_layout->addWidget(canvas);

    scrollArea->viewport()->setLayout(sa_layout);


    // enables on_<sender>_<signal>() mechanism
    QMetaObject::connectSlotsByName(this);
}


// void
// Gist::resetCanvas(TreeCanvas* canvas, TreeBuilder* builder, bool isRestarts) {
//     canvas->reset(isRestarts);
//     builder->reset(canvas->data, canvas->na);
// }

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

        VisualNode* p = n->getParent(*current_tc->na);
        if (p == NULL) {
            navUp->setEnabled(false);
            navRight->setEnabled(false);
            navLeft->setEnabled(false);
        } else {
            navUp->setEnabled(true);
            unsigned int alt = n->getAlternative(*current_tc->na);
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
    nodeStatInspector->node(*canvas->na,n,stats,finished); /// for single node stats
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
        hideSize->setEnabled(false);
        
        // labelBranches->setEnabled(false);
        // labelPath->setEnabled(false);
        analyzeSimilarSubtrees->setEnabled(false);
        // initComparison->setEnabled(false);

        // toggleStop->setEnabled(false);
        // unstopAll->setEnabled(false);

        center->setEnabled(false); /// ??
        exportPDF->setEnabled(false);
        exportWholeTreePDF->setEnabled(false);
        print->setEnabled(false);
        printSearchLog->setEnabled(false);

        bookmarkNode->setEnabled(false);
        bookmarksGroup->setEnabled(false);
        sndCanvas->setEnabled(true);
    } else {
        // stop->setEnabled(false);
        // reset->setEnabled(true);
        sndCanvas->setEnabled(true);

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
        analyzeSimilarSubtrees->setEnabled(true);
        if (n->getStatus() == UNDETERMINED) {
            /// TODO: disable based on active canvas' current node:
            // labelBranches->setEnabled(false); 
        } else {
            // labelBranches->setEnabled(!n->isHidden());
        }

        VisualNode* root = n;
        while (!root->isRoot())
            root = root->getParent(*canvas->na);
        NextSolCursor nsc(n, false, *canvas->na);
        PreorderNodeVisitor<NextSolCursor> nsv(nsc);
        nsv.run();
        navNextSol->setEnabled(nsv.getCursor().node() != root);

        NextSolCursor psc(n, true, *canvas->na);
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
Gist::emitChangeMainTitle(const std::string& file_name) {
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
    /// important! Expands hidden nodes and pentagons
    expand = new QAction("Expand", this);
    expand->setShortcut(QKeySequence("Return"));
    expand->setShortcutContext(Qt::ApplicationShortcut);
    
    stop = new QAction("Stop search", this);
    stop->setShortcut(QKeySequence("Esc"));

    reset = new QAction("Reset", this);
    reset->setShortcut(QKeySequence("Ctrl+R"));

    initComparison = new QAction("Compare", this);
    /// TODO: make unavailable when no data

    sndCanvas = new QAction("Allow second canvas", this);
    sndCanvas->setCheckable(true);
    sndCanvas->setChecked(true);
    /// TODO: set a shortcut

    showPixelTree = new QAction("Pixel Tree View", this);
    depthAnalysis = new QAction("Depth Analysis", this);
    followPath = new QAction("Follow Path", this);
    
    navUp = new QAction("Up", this);
    navUp->setShortcut(QKeySequence("Up"));
    navUp->setShortcutContext(Qt::ApplicationShortcut);
    
    navDown = new QAction("Down", this);
    navDown->setShortcut(QKeySequence("Down"));
    navDown->setShortcutContext(Qt::ApplicationShortcut);
    
    navLeft = new QAction("Left", this);
    navLeft->setShortcut(QKeySequence("Left"));
    navLeft->setShortcutContext(Qt::ApplicationShortcut);
    
    navRight = new QAction("Right", this);
    navRight->setShortcut(QKeySequence("Right"));
    navRight->setShortcutContext(Qt::ApplicationShortcut);
    
    navRoot = new QAction("Root", this);
    navRoot->setShortcut(QKeySequence("R"));
    navRoot->setShortcutContext(Qt::ApplicationShortcut);

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
    hideFailed->setShortcutContext(Qt::ApplicationShortcut);

    hideSize = new QAction("Hide small subtrees", this);
    
    unhideAll = new QAction("Unhide all", this);
    unhideAll->setShortcut(QKeySequence("U"));
    unhideAll->setShortcutContext(Qt::ApplicationShortcut);
    
    labelBranches = new QAction("Label/clear branches", this);
    labelBranches->setShortcut(QKeySequence("L"));
    labelBranches->setShortcutContext(Qt::ApplicationShortcut);
    
    labelPath = new QAction("Label/clear path", this);
    labelPath->setShortcut(QKeySequence("Shift+L"));
    labelPath->setShortcutContext(Qt::ApplicationShortcut);

    analyzeSimilarSubtrees = new QAction("Analyse similar subtrees", this);
    analyzeSimilarSubtrees->setShortcut(QKeySequence("Shift+s"));

    showNogoods = new QAction("Show no-goods", this);
    showNogoods->setShortcut(QKeySequence("Shift+N"));

    showNodeInfo = new QAction("Show node info", this);
    showNodeInfo->setShortcut(QKeySequence("I"));
    
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

    addAction(expand);
    addAction(stop);
    addAction(reset);
    addAction(sndCanvas);
    addAction(initComparison);
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
    addAction(depthAnalysis);
    addAction(followPath);
    addAction(analyzeSimilarSubtrees);
    addAction(showNogoods);
    addAction(showNodeInfo);
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
    contextMenu->addAction(analyzeSimilarSubtrees);
    contextMenu->addAction(showNogoods);
    contextMenu->addAction(showNodeInfo);

    contextMenu->addAction(toggleStop);
    contextMenu->addAction(unstopAll);

    contextMenu->addSeparator();

    contextMenu->addMenu(bookmarksMenu);

    contextMenu->addSeparator();
}

void
Gist::onFocusChanged(QWidget* a, QWidget* b) {

  // return;
  (void) a; // unused
  if (b) {
    if (QString(b->metaObject()->className()) == "QAbstractScrollArea"){
      QAbstractScrollArea* sa = static_cast<QAbstractScrollArea*>(b);
      QString window = sa->parentWidget()->metaObject()->className();
      if (window == "Gist") {
        Gist* gist = static_cast<Gist*>(sa->parentWidget());
        connectCanvas(gist->getCanvas());
      } else if (window == "QDialog" || window == "CmpTreeDialog" || window == "SolverTreeDialog")  {
        BaseTreeDialog* td = static_cast<BaseTreeDialog*>(sa->parentWidget());
        connectCanvas(td->getCanvas());
      }
      
    }
  }
}

void
Gist::connectCanvas(TreeCanvas* tc) {
    
    if (current_tc == tc) return;

    if (current_tc && current_tc->_builder) {

        disconnect(expand, SIGNAL(triggered()), current_tc, SLOT(expandCurrentNode()));
        disconnect(stop, SIGNAL(triggered()), current_tc, SLOT(stopSearch()));
        disconnect(reset, SIGNAL(triggered()), current_tc, SLOT(reset()));
        disconnect(sndCanvas, SIGNAL(triggered()), current_tc, SLOT(toggleSecondCanvas()));
        disconnect(navUp, SIGNAL(triggered()), current_tc, SLOT(navUp()));
        disconnect(navDown, SIGNAL(triggered()), current_tc, SLOT(navDown()));
        disconnect(navLeft, SIGNAL(triggered()), current_tc, SLOT(navLeft()));
        disconnect(navRight, SIGNAL(triggered()), current_tc, SLOT(navRight()));
        disconnect(navRoot, SIGNAL(triggered()), current_tc, SLOT(navRoot()));
        disconnect(navNextSol, SIGNAL(triggered()), current_tc, SLOT(navNextSol()));
        disconnect(navPrevSol, SIGNAL(triggered()), current_tc, SLOT(navPrevSol()));
        disconnect(toggleHidden, SIGNAL(triggered()), current_tc, SLOT(toggleHidden()));
        disconnect(hideFailed, SIGNAL(triggered()), current_tc, SLOT(hideFailed()));
        disconnect(hideSize, SIGNAL(triggered()), current_tc, SLOT(hideSize()));
        disconnect(labelBranches, SIGNAL(triggered()), current_tc, SLOT(labelBranches()));
        disconnect(unhideAll, SIGNAL(triggered()), current_tc, SLOT(unhideAll()));
        disconnect(labelPath, SIGNAL(triggered()), current_tc, SLOT(labelPath()));
        disconnect(analyzeSimilarSubtrees, SIGNAL(triggered()), current_tc, SLOT(analyzeSimilarSubtrees()));
        disconnect(showNogoods, SIGNAL(triggered()), current_tc, SLOT(showNogoods()));
        disconnect(showNodeInfo, SIGNAL(triggered()), current_tc, SLOT(showNodeInfo()));
        disconnect(toggleStop, SIGNAL(triggered()), current_tc, SLOT(toggleStop()));
        disconnect(unstopAll, SIGNAL(triggered()), current_tc, SLOT(unstopAll()));
        disconnect(zoomToFit, SIGNAL(triggered()), current_tc, SLOT(zoomToFit()));
        disconnect(center, SIGNAL(triggered()), current_tc, SLOT(centerCurrentNode()));
        disconnect(exportWholeTreePDF, SIGNAL(triggered()), current_tc, SLOT(exportWholeTreePDF()));
        disconnect(exportPDF, SIGNAL(triggered()), current_tc, SLOT(exportPDF()));
        disconnect(print, SIGNAL(triggered()), current_tc, SLOT(print()));
        disconnect(printSearchLog, SIGNAL(triggered()), current_tc, SLOT(printSearchLog()));
        disconnect(bookmarkNode, SIGNAL(triggered()), current_tc, SLOT(bookmarkNode()));
        disconnect(current_tc, SIGNAL(addedBookmark(const QString&)), this, SLOT(addBookmark(const QString&)));
        disconnect(current_tc, SIGNAL(removedBookmark(int)), this, SLOT(removeBookmark(int)));
        disconnect(current_tc, SIGNAL(needActionsUpdate(VisualNode*, bool)),
            this, SLOT(updateActions(VisualNode*, bool)));
    }

    current_tc = tc;

    /// TODO: these 2 should not be here
    connect(receiver, SIGNAL(startReceiving(void)),
            tc->_builder, SLOT(startBuilding(void)));
    connect(receiver, SIGNAL(doneReceiving(void)),
            tc->_builder, SLOT(setDoneReceiving(void)));
    connect(expand, SIGNAL(triggered()), tc, SLOT(expandCurrentNode()));
    connect(stop, SIGNAL(triggered()), tc, SLOT(stopSearch()));
    connect(reset, SIGNAL(triggered()), tc, SLOT(reset()));
    connect(sndCanvas, SIGNAL(triggered()), tc, SLOT(toggleSecondCanvas()));
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
    connect(depthAnalysis, SIGNAL(triggered()), tc, SLOT(depthAnalysis()));
    connect(followPath, SIGNAL(triggered()), tc, SLOT(followPath()));
    connect(analyzeSimilarSubtrees, SIGNAL(triggered()), tc, SLOT(analyzeSimilarSubtrees()));
    connect(showNogoods, SIGNAL(triggered()), current_tc, SLOT(showNogoods()));
    connect(showNodeInfo, SIGNAL(triggered()), current_tc, SLOT(showNodeInfo()));
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

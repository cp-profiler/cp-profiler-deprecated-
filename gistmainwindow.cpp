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

#include "gistmainwindow.h"
#include "preferences.hh"
#include "nodewidget.hh"
#include "treecanvas.hh"
#include "data.hh"

#include <cmath>
#include <fstream>

AboutGist::AboutGist(QWidget* parent) : QDialog(parent) {

  setMinimumSize(300, 240);
  setMaximumSize(300, 240);
  QVBoxLayout* layout = new QVBoxLayout();
  QLabel* aboutLabel =
    new QLabel(tr("<h2>Gist</h2>"
                   "<p><b>The Gecode Interactive Search Tool</b</p> "
                  "<p>You can find more information about Gecode and Gist "
                  "at</p>"
                  "<p><a href='http://www.gecode.org'>www.gecode.org</a>"
                  "</p"));
  aboutLabel->setOpenExternalLinks(true);
  aboutLabel->setWordWrap(true);
  aboutLabel->setAlignment(Qt::AlignCenter);
  layout->addWidget(aboutLabel);
  setLayout(layout);
  setWindowTitle(tr("About Gist"));
  setAttribute(Qt::WA_QuitOnClose, false);
  setAttribute(Qt::WA_DeleteOnClose, false);
}



GistMainWindow::GistMainWindow(Execution* execution, QWidget* parent) : QMainWindow(parent), aboutGist(this) {
    m_Gist = new Gist(execution, this);
  setCentralWidget(m_Gist);
  setWindowTitle(tr("CP-Profiler"));

//  Logos logos;
//  QPixmap myPic;
//  myPic.loadFromData(logos.gistLogo, logos.gistLogoSize);
//  setWindowIcon(myPic);

  resize(500,500);
  setMinimumSize(400, 200);

  menuBar = new QMenuBar(0);

  QMenu* fileMenu = menuBar->addMenu(tr("&File"));
  fileMenu->addAction(m_Gist->print);
  fileMenu->addAction(m_Gist->printSearchLog);
#if QT_VERSION >= 0x040400
  fileMenu->addAction(m_Gist->exportWholeTreePDF);
#endif

  prefAction = fileMenu->addAction(tr("Preferences"));
  connect(prefAction, SIGNAL(triggered()), this, SLOT(preferences()));

  QAction* quitAction = fileMenu->addAction(tr("Quit"));
  quitAction->setShortcut(QKeySequence("Ctrl+Q"));
  connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

  QMenu* nodeMenu = menuBar->addMenu(tr("&Node"));
  nodeMenu->addAction(m_Gist->showNodeStats);

  bookmarksMenu = new QMenu("Bookmarks", this);
  bookmarksMenu->addAction(m_Gist->bookmarkNode);
  connect(bookmarksMenu, SIGNAL(aboutToShow()),
          this, SLOT(populateBookmarks()));
  nodeMenu->addMenu(bookmarksMenu);
  nodeMenu->addSeparator();
  nodeMenu->addAction(m_Gist->navUp);
  nodeMenu->addAction(m_Gist->navDown);
  nodeMenu->addAction(m_Gist->navLeft);
  nodeMenu->addAction(m_Gist->navRight);
  nodeMenu->addAction(m_Gist->navRoot);
  nodeMenu->addAction(m_Gist->navNextSol);
  nodeMenu->addAction(m_Gist->navPrevSol);
  nodeMenu->addAction(m_Gist->navNextLeaf);
  nodeMenu->addAction(m_Gist->navPrevLeaf);
  nodeMenu->addSeparator();
  nodeMenu->addAction(m_Gist->toggleHidden);
  nodeMenu->addAction(m_Gist->hideFailed);
  nodeMenu->addAction(m_Gist->unhideAll);
  nodeMenu->addAction(m_Gist->labelBranches);
  nodeMenu->addAction(m_Gist->labelPath);
  nodeMenu->addAction(m_Gist->showNogoods);
  nodeMenu->addAction(m_Gist->showNodeInfo);

  nodeMenu->addSeparator();
  nodeMenu->addAction(m_Gist->zoomToFit);
  nodeMenu->addAction(m_Gist->center);
#if QT_VERSION >= 0x040400
  nodeMenu->addAction(m_Gist->exportPDF);
#endif

  /// ***** Tree Visualisaitons *****

  QMenu* treeVisMenu = menuBar->addMenu(tr("Tree"));

  treeVisMenu->addAction(m_Gist->highlightNodesMenu);
  treeVisMenu->addAction(m_Gist->analyzeSimilarSubtrees);
  treeVisMenu->addAction(m_Gist->showPixelTree);
  treeVisMenu->addAction(m_Gist->showIcicleTree);
  treeVisMenu->addAction(m_Gist->hideSize);
  treeVisMenu->addAction(m_Gist->followPath);
  treeVisMenu->addAction(m_Gist->deleteWhiteNodes);


  QMenu* helpMenu = menuBar->addMenu(tr("&Help"));
  QAction* aboutAction = helpMenu->addAction(tr("About"));
  connect(aboutAction, SIGNAL(triggered()),
          this, SLOT(about()));

  // Don't add the menu bar on Mac OS X
#ifndef Q_WS_MAC
  setMenuBar(menuBar);
#endif

  // Set up status bar
  QWidget* stw = new QWidget();
  QHBoxLayout* hbl = new QHBoxLayout();
  hbl->setContentsMargins(0,0,0,0);
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
  openLabel = new QLabel("     0");
  hbl->addWidget(openLabel);
  stw->setLayout(hbl);
  statusBar()->addPermanentWidget(stw);

  isSearching = false;
  statusBar()->showMessage("Ready");

  connect(m_Gist,SIGNAL(statusChanged(const Statistics&,bool)),
          this,SLOT(statusChanged(const Statistics&,bool)));

  connect(m_Gist, SIGNAL(changeMainTitle(QString)),
          this, SLOT(changeTitle(QString)));

  connect(this, SIGNAL(doneReceiving()), m_Gist, SIGNAL(doneReceiving()));

  // connect(this, SIGNAL(stopReceiver()), m_Gist->getReceiver(), SLOT(stopThread()));

  preferences(true);
  show();
  m_Gist->reset->trigger();
}

void
GistMainWindow::statusChanged(const Statistics& stats, bool finished) {
  if (stats.maxDepth==0) {
    isSearching = false;
    statusBar()->showMessage("Ready");
    prefAction->setEnabled(true);
  } else if (isSearching && finished) {
    isSearching = false;

    /// TODO(maxim): this should be done in here (delete include of data.hh)
    /// add total time to 'Done' label
    QString t;
    unsigned long long totalTime = m_Gist->getExecution()->getData()->getTotalTime();
    float seconds = totalTime / 1000000.0;
    t.setNum(seconds);
    statusBar()->showMessage("Done in " + t + "s");

    /// no need to change Status Bar anymore
    disconnect(m_Gist,SIGNAL(statusChanged(const Statistics&,bool)),
            this,SLOT(statusChanged(const Statistics&,bool)));


    prefAction->setEnabled(true);
  } else if (!isSearching && !finished) {
    // prefAction->setEnabled(false); /// TODO: leave active all the time instead?
    statusBar()->showMessage("Searching");
    isSearching = true;
  }
  depthLabel->setNum(stats.maxDepth);
  solvedLabel->setNum(stats.solutions);
  failedLabel->setNum(stats.failures);
  choicesLabel->setNum(stats.choices);
  openLabel->setNum(stats.undetermined);
}

void
GistMainWindow::about(void) {
  aboutGist.show();
}

void
GistMainWindow::preferences(bool setup) {
  PreferencesDialog pd(this);
  if (setup) {
    m_Gist->setAutoZoom(pd.zoom);
  }
  if (setup || pd.exec() == QDialog::Accepted) {
    m_Gist->setAutoHideFailed(pd.hideFailed);
    m_Gist->setRefresh(pd.refresh);
    m_Gist->setRefreshPause(pd.refreshPause);
    m_Gist->setSmoothScrollAndZoom(pd.smoothScrollAndZoom);
    m_Gist->setMoveDuringSearch(pd.moveDuringSearch);
  }
}

void
GistMainWindow::populateBookmarks(void) {
  bookmarksMenu->clear();
  bookmarksMenu->addAction(m_Gist->bookmarkNode);
  bookmarksMenu->addSeparator();
  bookmarksMenu->addActions(m_Gist->bookmarksGroup->actions());
}

void
GistMainWindow::changeTitle(QString file_name) {
  qDebug() << "changing title to: " << file_name;
  this->setWindowTitle(file_name);
}

void
GistMainWindow::gatherStatistics(void) {
  std::cerr << "GistMainWindow::gatherStatistics\n";
  std::ofstream out;
  out.open(statsFilename.toStdString(), std::ofstream::out);
  m_Gist->getCanvas()->collectMLStatsRoot(out);
  out.close();
}

void
GistMainWindow::selectNode(int gid) {
    m_Gist->getCanvas()->navigateToNodeById(gid);
}

void
GistMainWindow::selectManyNodes(QVariantList gids) {
    m_Gist->getCanvas()->unselectAll();
    for (int i = 0 ; i < gids.size() ; i++) {
        double d = gids[i].toDouble();
        int gid = (int) d;
        VisualNode* node = (m_Gist->getCanvas()->getExecution()->getNA())[gid];
        node->setSelected(true);
    }
    m_Gist->getCanvas()->updateCanvas();
}

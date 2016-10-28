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

#ifndef GISTMAINWINDOW_H
#define GISTMAINWINDOW_H


#include <QMainWindow>
#include <QVariant>
#include <memory>

class QLabel;
class QAction;
class NodeStatInspector;
class TreeCanvas;
class QGridLayout;
class Execution;
class QAction;
class QActionGroup;
class VisualNode;
class Statistics;
class ProfilerConductor;

class GistMainWindow : public QMainWindow {
  Q_OBJECT
private:

  QPalette* myPalette;
  QGridLayout* layout;

  std::unique_ptr<TreeCanvas> m_Canvas;

  ProfilerConductor& conductor;

  Execution& execution;
  /// Whether search is currently running
  bool isSearching = false;
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
  /// Menu for bookmarks
  QMenu* bookmarksMenu;
  /// Action for activating the preferences menu
  QAction* prefAction;
  /// A menu bar
  QMenuBar* menuBar;

  QString statsFilename;

  /// ****** FROM QTGIST ******

    /// Menu of comparators // TODO(maxim): what is it?
  QMenu* comparatorMenu;

    /// Context menu
  QMenu* contextMenu;

  QAction* reset;

  /// Group of all actions for bookmarks
  QActionGroup* bookmarksGroup;

  /// Action used when no bookmark exists
  QAction* nullBookmark;

  /// Print tree
  QAction* print;
  /// Print search tree log
  QAction* printSearchLog;
  /// Export PDF of whole tree
  QAction* exportWholeTreePDF;

  /// Bookmark current node
  QAction* bookmarkNode;
  /// Open node statistics inspector
  QAction* showNodeStats;

  /// Navigate to parent node
  QAction* navUp;
  /// Navigate to leftmost child node
  QAction* navDown;
  /// Navigate to left sibling
  QAction* navLeft;
  /// Navigate to right sibling
  QAction* navRight;
  /// Navigate to root node
  QAction* navRoot;
  /// Navigate to next solution (to the right)
  QAction* navNextSol;
  /// Navigate to previous solution (to the left)
  QAction* navPrevSol;
  /// Navigate to next leaf (to the right)
  QAction* navNextLeaf;
  /// Navigate to previous leaf (to the left)
  QAction* navPrevLeaf;

  /// Toggle whether current node is hidden
  QAction* toggleHidden;
  /// Hide failed subtrees under current node
  QAction* hideFailed;
  /// Unhide all hidden subtrees under current node
  QAction* unhideAll;

  /// Label branches under current node
  QAction* labelBranches;
  /// Label branches on path to root
  QAction* labelPath;

  /// Show no-goods
  QAction* showNogoods;
  /// Show node info
  QAction* showNodeInfo;

    /// Zoom tree to fit window
  QAction* zoomToFit;
  /// Center on current node
  QAction* center;

  /// Export PDF of current subtree
  QAction* exportPDF;

  /// Highlight nodes
  QAction* highlightNodesMenu;
  /// Analyze similar subtrees
  QAction* analyzeSimilarSubtrees;
  /// Show Indented Pixel Tree View
  QAction* showPixelTree;
    /// Show Icicle Search Tree
  QAction* showIcicleTree;
  /// Remove Unexplored Nodes
  QAction* deleteWhiteNodes;

  /// Hide subtrees by their size
  QAction* hideSize;
  /// Follow path
  QAction* followPath;

  /// Search next solution in current subtree
  QAction* searchNext;
  /// Search all solutions in current subtree
  QAction* searchAll;
  
  /// Show on a pixel tree
  QAction* showNodeOnPixelTree;
  
  /// Bookmark current node
  QAction* toggleStop;
  /// Bookmark current node
  QAction* unstopAll;
  /// Show webscript view
  QAction* showWebscript;

  /// Extract subtree into a fake execution
  QAction* extractSubtree;

  /// Group of all actions for comparators
  QActionGroup* comparatorGroup;

#ifdef MAXIM_DEBUG
  /// Delete a node from the tree
  QAction* deleteNode;

  QAction* dirtyUpNode;
#endif

    /// Information about individual nodes
  NodeStatInspector* nodeStatInspector;

    /// Action used when no comparator is registered
  QAction* nullComparator;

  /// *************************

  void addActions();

Q_SIGNALS:

  /// stops the receiver thread
  void stopReceiver(void);

  void doneReceiving(void);

  /// Notify MainWindow about fzn file name
  void changeMainTitle(QString file_name);

private Q_SLOTS:

  /// TODO: this should only react on selecting node?
  void updateActions(VisualNode*, bool);
    /// Reacts on status changes
  void statusChanged(VisualNode*, const Statistics&, bool);
    /// Displays the context menu for a node
  void onContextMenu(QContextMenuEvent*);
  /// Reacts on bookmark selection
  void selectBookmark(QAction*);
  /// Reacts on adding a bookmark
  void addBookmark(const QString& id);
  /// Reacts on removing a bookmark
  void removeBookmark(int idx);
  /// Populate the bookmarks menu
  void populateBookmarksMenu(void);
  /// Shows node status information
  void showStats(void);

protected Q_SLOTS:
  /// The status has changed (e.g., new solutions have been found)
  void statusChanged(const Statistics& stats, bool finished);
  /// Open the preferences dialog
  void preferences(bool setup=false);
  /// Populate the bookmarks menus from the actions found in Gist
  void populateBookmarks(void);
  /// Change MainWindow's title to fzn file name
public Q_SLOTS:
  void changeTitle(QString file_name);
  void gatherStatistics(void);

  void selectNode(int gid);
  void selectManyNodes(QVariantList gids);
public:
  GistMainWindow(Execution& execution, ProfilerConductor* parent);
  ~GistMainWindow();

  void setStatsFilename(QString filename) {
      statsFilename = filename;
  }

  /// Set preference whether to automatically hide failed subtrees
  void setAutoHideFailed(bool b);
  /// Set preference whether to automatically zoom to fit
  void setAutoZoom(bool b);
  /// Return preference whether to automatically hide failed subtrees
  bool getAutoHideFailed(void);
  /// Return preference whether to automatically zoom to fit
  bool getAutoZoom(void);

  /// Set refresh rate
  void setRefresh(int i);
  /// Set refresh pause in msec
  void setRefreshPause(int i);
  /// Return preference whether to use smooth scrolling and zooming
  bool getSmoothScrollAndZoom(void);
  /// Set preference whether to use smooth scrolling and zooming
  void setSmoothScrollAndZoom(bool b);
  /// Return preference whether to move cursor during search
  bool getMoveDuringSearch(void);
  /// Set preference whether to move cursor during search
  void setMoveDuringSearch(bool b);

  /// Handle resize event
  void resizeEvent(QResizeEvent*);

  /// Receiver calles this when name is obtained
  void emitChangeMainTitle(QString file_name);

  TreeCanvas* getCanvas();

};

#endif // MAINWINDOW_H

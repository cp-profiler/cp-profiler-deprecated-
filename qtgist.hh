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

#ifndef GIST_HH
#define GIST_HH

#include "nodestats.hh"
#include <vector>

/**
 * \brief %Gecode Interactive %Search Tool
 *
 * This class provides an interactive search tree viewer and explorer as
 * a Qt widget. You can embedd or inherit from this widget to use %Gist
 * in your own project.
 *
 * \ingroup TaskGist
 */

class CmpTreeDialog;
class TreeCanvas;
class Execution;

using std::vector;

class Gist : public QWidget {
  Q_OBJECT

  QPalette* myPalette;
  QGridLayout* layout;

  /// The canvas implementation
  TreeCanvas* m_Canvas;

  void addActions();

  Execution* execution;

private:
  /// The time slider
  QSlider* timeBar;
  /// Context menu
  QMenu* contextMenu;
  /// Action used when no comparator is registered
  QAction* nullComparator;
  /// Menu of comparators
  QMenu* comparatorMenu;
  /// Action used when no bookmark exists
  QAction* nullBookmark;
  /// Bookmark menu
  QMenu* bookmarksMenu;
  /// Information about individual nodes
  NodeStatInspector* nodeStatInspector;
  
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

  /// Group of all actions for comparators
  QActionGroup* comparatorGroup;
  

public:
  /// ------------ accessed from mainwindow --------------
  /// Reset %Gist
  QAction* reset;

  /// Group of all actions for bookmarks
  QActionGroup* bookmarksGroup;

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

  /// Hide subtrees by their size
  QAction* hideSize;
  /// Follow path
  QAction* followPath;

public:


  explicit Gist(Execution* execution, QWidget* parent);

  /// Destructor
  ~Gist(void);


  /// Set preference whether to automatically hide failed subtrees
  void setAutoHideFailed(bool b);
  /// Set preference whether to automatically zoom to fit
  void setAutoZoom(bool b);
  /// Return preference whether to automatically hide failed subtrees
  bool getAutoHideFailed(void);
  /// Return preference whether to automatically zoom to fit
  bool getAutoZoom(void);
  /// Set preference whether to show copies in the tree
  void setShowCopies(bool b);
  /// Return preference whether to show copies in the tree
  bool getShowCopies(void);

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


  /// ***** GETTERS *****
  TreeCanvas* getCanvas() { return m_Canvas; }
  Execution* getExecution() { return execution; }

Q_SIGNALS:

  /// Signals that the tree has changed
  void statusChanged(const Statistics&, bool);

  /// Signals that a solution has been found
  void solution(int);

  /// Notify MainWindow about fzn file name
  void changeMainTitle(QString file_name);

  void doneReceiving(void);


private Q_SLOTS:
  /// Displays the context menu for a node
  void on_canvas_contextMenu(QContextMenuEvent*);
  /// Reacts on status changes
  void on_canvas_statusChanged(VisualNode*, const Statistics&, bool);
  /// TODO: this should only react on selecting node?
  void updateActions(VisualNode*, bool);
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

};


#endif // GIST_HH

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

#include "treecanvas.hh"
#include "treecomparison.hh"
#include "nodestats.hh"
#include "receiverthread.hh"
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

class SolverTreeDialog;
class CmpTreeDialog;

using std::vector;

class Gist : public QWidget {
  Q_OBJECT

  friend ReceiverThread;

  /// **************** INTERFACE *******************

  QAbstractScrollArea* scrollArea;
  QPalette* myPalette;
  QGridLayout* layout;

  /// tree dialogs for additional solver runs
  vector<SolverTreeDialog*> _td_vec;


  /// **************** MY STUFF ********************
private:

  /// Checks for new nodes
  ReceiverThread* receiver;

  /// Points to the active canvas
  TreeCanvas* current_tc = nullptr;

  /// The canvas implementation
  TreeCanvas* canvas;

  /// Second canvas in case of comparing
  TreeCanvas* canvasTwo;

  /// Merged tree (comparison)
  TreeCanvas* cmpCanvas;

  /// Dialog for a merged tree (comparison)
  QDialog* cmpDialog;

  void initInterface(void);
  void addActions(void);

  /// connect the signals as well as disconnect current_tc
  void connectCanvas(TreeCanvas* tc);

public:

  /// Reset treeCanvas ( reset TreeBuilder, reset Canvas itself)
  /// This should be called instead of TreeCanvas::reset
  // void resetCanvas(TreeCanvas* canvas, TreeBuilder* builder, bool isRestarts);

  /// ***********************************************
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
public:
  /// Expand current node
  QAction* expand;
  /// Stop search
  QAction* stop;
  /// Reset %Gist
  QAction* reset;
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
  /// Navigate to next solution (to the left)
  QAction* navNextSol;
  /// Navigate to previous solution (to the right)
  QAction* navPrevSol;
  /// Search next solution in current subtree
  QAction* searchNext;
  /// Search all solutions in current subtree
  QAction* searchAll;
  /// Toggle whether current node is hidden
  QAction* toggleHidden;
  /// Hide failed subtrees under current node
  QAction* hideFailed;
  /// Hide subtrees by their size
  QAction* hideSize;
  /// Unhide all hidden subtrees under current node
  QAction* unhideAll;
  /// Label branches under current node
  QAction* labelBranches;
  /// Label branches on path to root
  QAction* labelPath;
  /// Analyze similar subtrees
  QAction* analyzeSimilarSubtrees;
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
  /// Export PDF of whole tree
  QAction* exportWholeTreePDF;
  /// Print tree
  QAction* print;
  /// Print search tree log
  QAction* printSearchLog;
  /// Creates new canvas and runs comparison tool
  QAction* initComparison;
  /// Allow second canvas (when new data received)
  QAction* sndCanvas;

  /// Bookmark current node
  QAction* bookmarkNode;
  /// Open node statistics inspector
  QAction* showNodeStats;
  /// Bookmark current node
  QAction* toggleStop;
  /// Bookmark current node
  QAction* unstopAll;


  /// Show Indented Pixel Tree View
  QAction* showPixelTree;

  /// Perform depth analysis on a tree
  QAction* depthAnalysis;
  /// Follow path
  QAction* followPath;

  /// Group of all actions for comparators
  QActionGroup* comparatorGroup;
  /// Group of all actions for bookmarks
  QActionGroup* bookmarksGroup;

public:
  /// Constructor
//  Gist(Space* root, bool bab, QWidget* parent, const Options& opt);
  Gist(QWidget* parent);
  /// Destructor
  ~Gist(void);

//  /// Add comparator \a c0
//  void addComparator(Comparator* c0);

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

  /// Stop search and wait until finished
  bool finish(void);

  /// Handle resize event
  void resizeEvent(QResizeEvent*);

  /// Receiver calles this when name is obtained
  void emitChangeMainTitle(const std::string& file_name);


  /// ***** GETTERS *****
  TreeCanvas* getCanvas(void) { return canvas; }
  ReceiverThread* getReceiver(void) { return receiver; }

  /// returns canvas from td_vec (additional canvases)
  TreeCanvas* getLastCanvas(void);
  SolverTreeDialog* getLastTreeDialog(void);

Q_SIGNALS:

  /// Signals that the tree has changed
  void statusChanged(const Statistics&, bool);

  /// Signals that a solution has been found
  void solution(int);

  /// Notify MainWindow about fzn file name
  void changeMainTitle(const std::string& file_name);


private Q_SLOTS:
  /// Create new TreeCanvas based on comparison
  void initiateComparison(void);
  /// Create new TreeCanvas if already have one
  void createNewCanvas(void);
  /// Create new TreeCanvas if already have one
  void prepareNewCanvas(void); // TODO: get rid of this one
  /// Displays the context menu for a node
  void on_canvas_contextMenu(QContextMenuEvent*);
  /// Reacts on status changes
  void on_canvas_statusChanged(VisualNode*, const Statistics&, bool);


  /// TODO: this should only react on selecting node?
  void updateActions(VisualNode*, bool);
//  /// Reacts on comparator selection
//  void selectComparator(QAction*);
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


public Q_SLOTS:
  void onFocusChanged(QWidget*, QWidget*);

};


#endif // GIST_HH

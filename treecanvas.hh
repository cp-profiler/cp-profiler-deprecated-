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


#ifndef TREECANVAS_HH
#define TREECANVAS_HH

#include <QtGui>
#include <QtWidgets>

#include <functional>
#include "visualnode.hh"
#include "execution.hh"

/// \brief Parameters for the tree layout
namespace LayoutConfig {
  /// Minimum scale factor
  constexpr int minScale = 1;
  /// Maximum scale factor
  constexpr int maxScale = 400;
  /// Default scale factor
  constexpr int defScale = 100;
  /// Maximum scale factor for automatic zoom
  constexpr int maxAutoZoomScale = defScale;
}

class TreeCanvas;

namespace cpprofiler { namespace analysis {
  class SimilarShapesWindow;
}}

class TreeDialog;

enum class CanvasType {
  REGULAR,
  MERGED
};



/// \brief A canvas that displays the search tree
class TreeCanvas : public QWidget {
  Q_OBJECT

  /// TODO: try to reduce the number of these
  friend class Gist;
  friend class ShapeCanvas;
  friend class TreeComparison;
  friend class CmpTreeDialog;

  /// to generate ids
  static int counter;

  int _id;

  Execution& execution;

  int nodeCount = 0;
  QTimer* updateTimer;

  /// Mutex for synchronizing acccess to the tree
  QMutex& mutex;
  /// Mutex for synchronizing layout and drawing
  QMutex& layoutMutex;
  /// Flag signalling that Gist is ready to be closed
  bool finishedFlag;
  /// Allocator for nodes
  NodeAllocator& na;
  /// The root node of the tree
  VisualNode* root;
  /// The currently selected node
  VisualNode* currentNode = nullptr;
  /// The head of the currently selected path
  VisualNode* pathHead;

  /// The bookmarks map
  QVector<VisualNode*> bookmarks;

  /// The scale bar
  QSlider* scaleBar;

  /// Box for selecting "small subtree" size
  QLineEdit* smallBox;

  /// Current scale factor
  double scale;
  /// Offset on the x axis so that the tree is centered
  int xtrans;

  /// Whether to hide failed subtrees automatically
  bool autoHideFailed = true;
  /// Whether to zoom automatically
  bool autoZoom = false;
  /// Whether to show copies in the tree
  bool showCopies;
  /// Refresh rate
  int refresh = 500;
  /// Time (in msec) to pause after each refresh
  int refreshPause = 0;
  /// Whether to use smooth scrolling and zooming
  bool smoothScrollAndZoom = false;
  /// Whether to move cursor during search
  bool moveDuringSearch = false;
  /// Timer for smooth zooming
  QTimeLine zoomTimeLine{500};
  /// Timer for smooth scrolling
  QTimeLine scrollTimeLine{1000};
  /// Target x coordinate after smooth scrolling
  int targetX = 0;
  /// Source x coordinate after smooth scrolling
  int sourceX = 0;
  /// Target y coordinate after smooth scrolling
  int targetY = 0;
  /// Target y coordinate after smooth scrolling
  int sourceY = 0;

  /// Target width after layout
  int targetW = 0;
  /// Target height after layout
  int targetH = 0;
  /// Target scale after layout
  int targetScale = 0;
  /// Timer id for delaying the update
  int layoutDoneTimerId = 0;

    /// Return the node corresponding to the \a event position
  VisualNode* eventNode(QEvent *event);
  /// General event handler, used for displaying tool tips
  bool event(QEvent *event);
  /// Paint the tree
  void paintEvent(QPaintEvent* event);
  /// Handle mouse press event
  void mousePressEvent(QMouseEvent* event);
  /// Handle mouse double click event
  void mouseDoubleClickEvent(QMouseEvent* event);
  /// Handle context menu event
  void contextMenuEvent(QContextMenuEvent* event);
  /// Handle resize event
  void resizeEvent(QResizeEvent* event);
  /// Handle mouse wheel events
  void wheelEvent(QWheelEvent* event);

  /// Timer invoked for smooth zooming and scrolling
  virtual void timerEvent(QTimerEvent* e);
  /// Similar shapes dialog
  std::unique_ptr<cpprofiler::analysis::SimilarShapesWindow> shapesWindow;
  // Node that represents the shape currently selected
  VisualNode* shapeHighlighted;

public:

  TreeCanvas(Execution* execution, QGridLayout* layout, CanvasType type, QWidget* parent);

  ~TreeCanvas();

  std::string getLabel(unsigned int gid) {
    return execution.getLabel(gid);
  }
  unsigned long long getTotalTime() const { return execution.getTotalTime(); }
  std::string getTitle() const { return execution.getTitle(); }
  DbEntry* getEntry(unsigned int gid) { return execution.getEntry(gid); }

  const Statistics& get_stats() const { return execution.getStatistics(); }

  Execution* getExecution() const { return &execution; }

  unsigned int getTreeDepth();

  void printSearchLogTo(const QString& file_name);

  /// Apply `action` to every node that satisfies the predicate
  void applyToEachNodeIf(std::function<void (VisualNode*)> action,
                         std::function<bool (VisualNode*)> predicate);

  /// traverse every node and set hidden
  void hideAll();

Q_SIGNALS:

  void scaleChanged(int);
  /// The auto-zoom state was changed
  void autoZoomChanged(bool);
  /// Context menu triggered
  void contextMenu(QContextMenuEvent*);
  /// Status bar update
  void statusChanged(VisualNode*, const Statistics&, bool);

  void needActionsUpdate(VisualNode*, bool);
  /// Signals that a solution has been found
  void solution(int);
  /// Signals that %Gist is finished
  void searchFinished();
  /// Signals that a bookmark has been added
  void addedBookmark(const QString& id);
  /// Signals that a bookmark has been removed
  void removedBookmark(int idx);

  void showNodeOnPixelTree(int gid);

  void announceSelectNode(int gid);

public Q_SLOTS:
  void reset();

  void maybeUpdateCanvas();
  void updateCanvas();
  /// Update display
  void update();
  /// React to scroll events
  void scroll();
  /// Layout done
  void layoutDone(int w, int h, int scale0);
  /// Set the selected node to \a n
  void setCurrentNode(VisualNode* n, bool finished=true, bool update=true);
  /// Set the selected not to a node by solver id (from no-good table)
  void navigateToNodeById(int gid);
  void statusFinished();

  void deleteNode(Node* n);

  /// Set scale factor to \a scale0
  void scaleTree(int scale0, int zoomx=-1, int zoomy=-1);

  /// Toggle hidden state of selected node
  void toggleHidden();
  /// Hide failed subtrees of selected node
  void hideFailed();
  /// Hide subtrees under a certain size
  void hideSize();
  /// Unhide all nodes below selected node
  void unhideAll();
  /// Unselect all nodes
  void unselectAll();
  /// Sets the node and its ancestry as not hidden;
  /// marks the path as dirty
  void unhideNode(VisualNode* node);
  /// Do not stop at selected stop node
  void toggleStop();
  /// Do not stop at any stop node
  void unstopAll();
  /// Export pdf of the current subtree
  void exportPDF();
  /// Export pdf of the whole tree
  void exportWholeTreePDF();
  /// Print the tree
  void print();
  /// Print the search tree log
  void printSearchLog();
  /// Zoom the canvas so that the whole tree fits
  void zoomToFit();
  /// Center the view on the currently selected node
  void centerCurrentNode();
  /// Expand hidden node or pentagon
  void expandCurrentNode();
  /// Label all branches in subtree under current node
  void labelBranches();
  /// Label all branches on path to root node
  void labelPath();

  /// Show Indented Pixel Tree View
  void showPixelTree();

  /// Show Icicle Tree View
  void showIcicleTree();

  /// Delete Unexplored Nodes (needed e.g. for replaying with restart)
  void deleteWhiteNodes();

  /// Follow path from root
  void followPath();

  /// Analyze similar subtrees of current node
  void analyzeSimilarSubtrees();

  /// Show highlight nodes dialog
  void highlightNodesMenu();

  /// Show no-goods
  void showNogoods();

  /// Show node info
  void showNodeInfo();

  /// Show a node on a pixel tree
  void showNodeOnPixelTree();

  /// Collect ML stats from the current node
  void collectMLStats();
  /// Collect ML stats for a specified node
  void collectMLStats(VisualNode* node);
  /// Collect ML stats from the root
  void collectMLStatsRoot(std::ostream& out);

  void highlightSubtrees(std::vector<VisualNode*>& nodes);

  void resetNodesHighlighting();

  /// highlight nodes with non-empty info field
  void highlightNodesWithInfo();

  /// highlight failures caused by nogoods (nogoods in info)
  void highlightFailedByNogoods();

  /// Move selection to the parent of the selected node
  void navUp();
  /// Move selection to the first child of the selected node
  void navDown();
  /// Move selection to the left sibling of the selected node
  void navLeft();
  /// Move selection to the right sibling of the selected node
  void navRight();
  /// Move selection to the root node
  void navRoot();
  /// Move selection to next solution (in DFS order)
  void navNextSol(bool back = false);
  /// Move selection to next leaf (in DFS order)
  void navNextLeaf(bool back = false);
  /// Move selection to next pentagon (in DFS order)
  void navNextPentagon(bool back = false);
  /// Move selection to previous solution (in DFS order)
  void navPrevSol();
  /// Move selection to previous leaf (in DFS order)
  void navPrevLeaf();
  /// Bookmark current node
  void bookmarkNode();
  /// Re-emit status change information for current node
  void emitStatusChanged();

  /// Set preference whether to automatically hide failed subtrees
  void setAutoHideFailed(bool b);
  /// Set preference whether to automatically zoom to fit
  void setAutoZoom(bool b);
  /// Return preference whether to automatically hide failed subtrees
  bool getAutoHideFailed();
  /// Return preference whether to automatically zoom to fit
  bool getAutoZoom();
  /// Set refresh rate
  void setRefresh(int i);
  /// Set refresh pause in msec
  void setRefreshPause(int i);
  /// Return preference whether to use smooth scrolling and zooming
  bool getSmoothScrollAndZoom();
  /// Set preference whether to use smooth scrolling and zooming
  void setSmoothScrollAndZoom(bool b);
  /// Return preference whether to move cursor during search
  bool getMoveDuringSearch();
  /// Set preference whether to move cursor during search
  void setMoveDuringSearch(bool b);
  /// Resize to the outer widget size if auto zoom is enabled
  void resizeToOuter();

  /// Stop search and wait for it to finish
  bool finish();

#ifdef MAXIM_DEBUG
  void printDebugInfo();
  void addChildren();
  void deleteSelectedNode();
  void dirtyUpNode();
#endif
private Q_SLOTS:
  /// Set isUsed to true and update
  void finalizeCanvas();
  /// Search has finished
  void statusChanged(bool);
  /// Export PDF of the subtree of \a n
  void exportNodePDF(VisualNode* n);
  /// Scroll to \a i percent of the target
  void scroll(int i);

  void updateViaTimer();
};

#endif // TREECANVAS_HH

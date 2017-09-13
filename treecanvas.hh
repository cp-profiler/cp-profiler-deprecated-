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

#include "namemap.hh"
#include <QtGui>
#include <QtWidgets>
#include <memory>
#include <sstream>
#include <functional>
#include <unordered_map>

/// \brief Parameters for the tree layout
namespace LayoutConfig {
  /// Minimum scale factor
  constexpr int minScale = 1;
  /// Maximum scale factor
  constexpr int maxScale = 200;
  /// Maximum scale factor for automatic zoom
  constexpr int maxAutoZoomScale = 100;
}

class Execution;
class Data;
class TreeCanvas;
class NodeTree;
class NodeAllocator;
class VisualNode;
class Node;

namespace cpprofiler { namespace analysis {
  class SimilarShapesWindow;
}}

class TreeDialog;

/// TODO(maxim): slowing down search does not work anymore?

/// \brief A canvas that displays the search tree
class TreeCanvas : public QWidget {
  Q_OBJECT

  struct DisplayOptions {
    /// Whether to hide failed subtrees automatically
    bool autoHideFailed = true;
    /// Whether to zoom automatically
    bool autoZoom = false;
    /// Refresh rate
    int refreshRate = 500;
    /// Time (in msec) to pause after each refresh
    int refreshPause = 0;
    /// Whether to use smooth scrolling and zooming
    bool smoothScrollAndZoom = false;
    /// Whether to move cursor during search
    bool moveDuringSearch = false;
    /// Current scale factor
    double scale;
  };

  struct ViewState {
    /// Target x coordinate after smooth scrolling
    int targetX = 0;
    /// Source x coordinate after smooth scrolling
    int sourceX = 0;
    /// Target y coordinate after smooth scrolling
    int targetY = 0;
    /// Target y coordinate after smooth scrolling
    int sourceY = 0;
    /// Target scale after layout
    int targetScale = 0;
    /// Offset on the x axis so that the tree is centered
    int xtrans;
  };

  friend class GistMainWindow;
  friend class Gist;

  Execution& execution;

  int nodeCount = 0;
  QTimer* updateTimer;

  DisplayOptions m_options;
  ViewState m_view;

  /// Mutex for synchronizing acccess to the tree
  QMutex& mutex;
  /// Mutex for synchronizing layout and drawing
  QMutex& layoutMutex;
  /// Allocator for nodes
  NodeAllocator& na;
  /// The root node of the tree
  VisualNode* root;
  /// The currently selected node
  VisualNode* currentNode = nullptr;

    /// Similar shapes dialog
  std::unique_ptr<cpprofiler::analysis::SimilarShapesWindow> shapesWindow;
  
  /// The bookmarks map
  QVector<VisualNode*> bookmarks;

  /// TODO(maxim): should be taken out to gist
  /// The scale bar
  QSlider* m_scaleBar;

  /// Box for selecting "small subtree" size
  QLineEdit* smallBox;

  /// Timer for smooth zooming
  QTimeLine zoomTimeLine{500};
  /// Timer for smooth scrolling
  QTimeLine scrollTimeLine{1000};

  /// Filter used by Node Info Dialog
  QString domain_filter{""};

  /// Timer id for delaying the update
  int layoutDoneTimerId = 0;

  /// Store mapping from id to path
  std::unordered_map<std::string, std::string> pathmap;

    /// Return the node corresponding to the \a event position
  VisualNode* eventNode(QEvent *event);
  /// General event handler, used for displaying tool tips
  bool event(QEvent *event) override;
  /// Paint the tree
  void paintEvent(QPaintEvent* event) override;
  /// Handle mouse press event
  void mousePressEvent(QMouseEvent* event) override;
  /// Handle mouse double click event
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  /// Handle context menu event
  void contextMenuEvent(QContextMenuEvent* event) override;
  /// Handle resize event
  void resizeEvent(QResizeEvent* event) override;
  /// Handle mouse wheel events
  void wheelEvent(QWheelEvent* event) override;

  /// Timer invoked for smooth zooming and scrolling
  virtual void timerEvent(QTimerEvent* e) override;

public:


  TreeCanvas(Execution* execution, QWidget* parent);

  ~TreeCanvas();

  QSlider* scaleBar() const { return m_scaleBar; }

  std::string getLabel(int gid);

  Execution& getExecution() const { return execution; }

  /// Apply `action` to every node that satisfies the predicate
  void applyToEachNodeIf(std::function<void (VisualNode*)> action,
                         std::function<bool (VisualNode*)> predicate);

  /// Apply `action` to every node that satisfies the predicate
  void applyToEachNode(std::function<void (VisualNode*)> action);

  /// Aplly `action` to every node with post-order traversal
  void applyToEachNodePO(std::function<void (VisualNode*)> action);

  /// traverse every node and set hidden
  void hideAll();

  std::pair<std::unique_ptr<NodeTree>, std::unique_ptr<Data>> extractSubtree();

  void findSelectedShape();

  // Send nogood heatmap to IDE
  void emitShowNogoodToIDE(const QString& heatmap, const QString& text, bool record) const;

Q_SIGNALS:

  void scaleChanged(int);
  /// The auto-zoom state was changed
  void autoZoomChanged(bool);
  /// Context menu triggered
  void contextMenu(QContextMenuEvent*);

  /// Signals that a bookmark has been added
  void addedBookmark(const QString& id);
  /// Signals that a bookmark has been removed
  void removedBookmark(int idx);

  void showNodeOnPixelTree(int gid);

  void announceSelectNode(int gid);

  void nodeSelected(VisualNode*);

  void moreNodesDrawn();

  void showNodeInfo(std::string extra_info);
  void showNogood(const QString& heatmap, const QString& text, bool record) const;

  void searchLogReady(const QString& replayPath);

public Q_SLOTS:
  void reset();

  void maybeUpdateCanvas();
  void updateCanvas(bool hide_failed = false);
  /// React to scroll events
  void scroll();
  /// Set the selected node to \a n
  void setCurrentNode(VisualNode* n, bool finished=true, bool update=true);

  const VisualNode* getCurrentNode() const { return currentNode; }
  VisualNode* getCurrentNode() { return currentNode; }
  /// Set the selected not to a node by solver id (from no-good table)
  void navigateToNodeById(int gid);

  /// Delete the node and all its descendants
  /// not sure if memory is properly released here
  void deleteNode(VisualNode* n);

  /// Delete the node and its left child, while reparenting its (alt) child
  void deleteMiddleNode(Node* n, int alt);

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

  /// Delete (immediately) unsuccessful assignments in the middle (chains)
  void deleteTrials();

  /// Delete unexplored and skipped nodes etc.
  void finalize();

  /// Delete Skipped Nodes (needed e.g. for replaying with restart)
  void deleteSkippedNodes();

  /// Follow path from root
  void followPath();

  /// Analyze similar subtrees of current node
  void analyzeSimilarSubtrees();

  /// Analyze similar subtrees of current node
  void compareSubtreeLabels();

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

  /// Write path to `node` into `str`
  void printPath(std::stringstream& str, const VisualNode* node);

  /// Print paths to `nodes`
  void printPaths(const std::vector<VisualNode*>& nodes);

  /// Print paths to highlighted subtrees
  void printHightlightedPaths();

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

  void analyseBackjumps();

  void openNodeSearch();

#ifdef MAXIM_DEBUG
private:
  void _addChildren(VisualNode* n);
public:
  void printDebugInfo();
  void addChildren();
  void nextStatus();
  void highlightSubtree();
  void createRandomTree(); // in place
  void deleteSelectedNode();
  void deleteSelectedMiddleNode();
  void dirtyUpNode();
#endif
private Q_SLOTS:
  /// Export PDF of the subtree of \a n
  void exportNodePDF(VisualNode* n);
  /// Scroll to \a i percent of the target
  void scroll(int i);

  void updateViaTimer();
};

#endif // TREECANVAS_HH

#ifndef TREECANVAS_HH
#define TREECANVAS_HH

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
// #include <QTimer>
#endif

#include <set>
#include <zmq.hpp>
#include "visualnode.hh"
#include "treebuilder.hh"

/// \brief Parameters for the tree layout
namespace LayoutConfig {
  /// Minimum scale factor
  const int minScale = 10;
  /// Maximum scale factor
  const int maxScale = 400;
  /// Default scale factor
  const int defScale = 100;
  /// Maximum scale factor for automatic zoom
  const int maxAutoZoomScale = defScale;
}

class TreeCanvas;
class SimilarShapesWindow;
class TreeBuilder;

/// *********************
/// SIMILAR SUBTREES
/// *********************
class ShapeCanvas : public QWidget {
  Q_OBJECT
public:
  ShapeCanvas(QWidget* parent = 0, TreeCanvas* tc = 0);
  VisualNode* _targetNode; // why _?

private:
  TreeCanvas* _tc;

  int xtrans;

  // width and height of the shape
  int width, height;

protected:
  /// Paint the shape
  void paintEvent(QPaintEvent* event);
protected Q_SLOTS:
  void scroll(void);
};

class ShapeI {
public:
  int sol;
  VisualNode* node;
  Shape* s;
  ShapeI(int sol0, VisualNode* node0)
    : sol(sol0), node(node0), s(Shape::copy(node->getShape())) {}
  ~ShapeI(void) { Shape::deallocate(s); }
  ShapeI(const ShapeI& sh) : sol(sh.sol), node(sh.node), s(Shape::copy(sh.s)) {}
  ShapeI& operator =(const ShapeI& sh) {
    if (this!=&sh) {
      Shape::deallocate(s);
      s = Shape::copy(sh.s);
      sol = sh.sol;
      node = sh.node;
    }
    return *this;
  }
};

class Filters {
public:
  Filters(TreeCanvas* tc);
  void setMinDepth(int);
  void setMinCount(int);
  bool apply(const ShapeI& s);
private:
  int _minDepth;
  int _minCount;
  TreeCanvas* _tc;
};

class ShapeRect : public QGraphicsRectItem {
public:
  ShapeRect(qreal, qreal, qreal, qreal, VisualNode*, SimilarShapesWindow*, QGraphicsItem*);
  VisualNode* getNode(void);
  // add to the scene
  void draw(QGraphicsScene* scene);
  QGraphicsRectItem selectionArea;
protected:
  void mousePressEvent (QGraphicsSceneMouseEvent*);
private:
  VisualNode* _node;
  ShapeCanvas* _shapeCanvas;
  SimilarShapesWindow* _ssWindow;
};

class SimilarShapesWindow : public QDialog {
  Q_OBJECT
public:
  SimilarShapesWindow(QWidget* parent = 0, TreeCanvas* tc = 0);
  ~SimilarShapesWindow(void);
  void drawHistogram(void);

  TreeCanvas* tc;
  ShapeCanvas* shapeCanvas;

public Q_SLOTS:
  void depthFilterChanged(int val);
  void countFilterChanged(int val);
private:
  void applyLayouts(void);

  QSplitter splitter;

  QVBoxLayout globalLayout;
  QHBoxLayout filtersLayout;
  QHBoxLayout depthFilterLayout;
  QHBoxLayout countFilterLayout;

  QGraphicsScene histScene;
  QGraphicsView view;
  QAbstractScrollArea scrollArea;

  Filters filters;

  QSpinBox depthFilterSB;
  QSpinBox countFilterSB;
};

/// less operator needed for the map
struct CompareShapes {
private:
  TreeCanvas& _tc;
public:
  CompareShapes(TreeCanvas& tc);
  bool operator()(const ShapeI& s1, const ShapeI& s2) const;
};



/// \brief A thread that concurrently explores the tree
class SearcherThread : public QThread {
  Q_OBJECT
private:
  VisualNode* node;
  int depth;
  bool a;
  TreeCanvas* t;
  
public:
  void search(VisualNode* n, bool all, TreeCanvas* ti);

public Q_SLOTS:
  void updateCanvas(void);

Q_SIGNALS:
  void update(int w, int h, int scale0);
  void statusChanged(bool);
  void scaleChanged(int);
  void solution(void);
  void searchFinished(void);
  void moveToNode(VisualNode* n,bool);
  void startWork(void);

protected:
  void run(void);
};

/// \brief A canvas that displays the search tree
class TreeCanvas : public QWidget {
  Q_OBJECT

  friend class SearcherThread;
  friend class Gist;
  friend class TreeBuilder;
  friend class ShapeCanvas;
  // friend class Data;

public:
  /// Constructor
  TreeCanvas(QWidget* parent);
  /// Destructor
  ~TreeCanvas(void);

  // data from the db
  QTimer* timer;
  Data* data;
  TreeBuilder* treeBuilder;

  

//  /// Add inspector \a i
//  void addDoubleClickInspector(Inspector* i);
//  /// Set active inspector
//  void activateDoubleClickInspector(int i, bool active);
//  /// Add inspector \a i
//  void addSolutionInspector(Inspector* i);
//  /// Set active inspector
//  void activateSolutionInspector(int i, bool active);
//  /// Add inspector \a i
//  void addMoveInspector(Inspector* i);
//  /// Set active inspector
//  void activateMoveInspector(int i, bool active);
//  /// Add comparator \a c
//  void addComparator(Comparator* c);
//  /// Set active comparator
//  void activateComparator(int i, bool active);

  int getNoOfSolvedLeaves(VisualNode& n);  // TODO: duplicate?
  /// Return number of solved children in the node
  int getNoOfSolvedLeaves(VisualNode* node);
  

public Q_SLOTS: 
  /// Set scale factor to \a scale0
  void scaleTree(int scale0, int zoomx=-1, int zoomy=-1);

  /// Explore complete subtree of selected node
  void searchAll(void);
  /// Find next solution below selected node
  void searchOne(void);
  /// Toggle hidden state of selected node
  void toggleHidden(void);
  /// Hide failed subtrees of selected node
  void hideFailed(void);
  /// Unhide all nodes below selected node
  void unhideAll(void);
  /// Do not stop at selected stop node
  void toggleStop(void);
  /// Do not stop at any stop node
  void unstopAll(void);
  /// Export pdf of the current subtree
  void exportPDF(void);
  /// Export pdf of the whole tree
  void exportWholeTreePDF(void);
  /// Print the tree
  void print(void);
  /// Zoom the canvas so that the whole tree fits
  void zoomToFit(void);
  /// Center the view on the currently selected node
  void centerCurrentNode(void);
  /**
   * \brief Call the double click inspector for the currently selected node
   *
   * If \a fix is true, then the node is inspected after fixpoint
   * computation, otherwise its status after branching but before
   * fixpoint computation is inspected.
   */
  void inspectCurrentNode(bool fix=true, int inspectorNo=-1);
  /// Calls inspectCurrentNode(false)
  void inspectBeforeFP(void);
  /// Label all branches in subtree under current node
  void labelBranches(void);
  /// Label all branches on path to root node
  void labelPath(void);

  /// Analyze similar subtrees of current node
  void analyzeSimilarSubtrees(void);

  /// calls when clicking right mouse button on a shape
  void highlightShape(VisualNode* node);

  /// Loop through all nodes and add them to the multimap
  void addNodesToMap(void);

  /// Stop current search
  void stopSearch(void);

  /// Reset, isRestarts true if we want a dummy node (needed for showing restarts)
  void reset(bool isRestarts = false);

  /// Do not redraw, but compare with the next tree
  void compareTrees(void);

  /// Move selection to the parent of the selected node
  void navUp(void);
  /// Move selection to the first child of the selected node
  void navDown(void);
  /// Move selection to the left sibling of the selected node
  void navLeft(void);
  /// Move selection to the right sibling of the selected node
  void navRight(void);
  /// Move selection to the root node
  void navRoot(void);
  /// Move selection to next solution (in DFS order)
  void navNextSol(bool back = false);
  /// Move selection to previous solution (in DFS order)
  void navPrevSol(void);

  /// Bookmark current node
  void bookmarkNode(void);
  /// Set the current node to be the head of the path
  void setPath(void);
  /// Call the double click inspector for all nodes on the path from root to head of the path
  void inspectPath(void);
  /// Wait for click on node to compare with current node
  void startCompareNodes(void);
  /// Wait for click on node to compare with current node before fixpoint
  void startCompareNodesBeforeFP(void);

  /// Re-emit status change information for current node
  void emitStatusChanged(void);

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
  /// Resize to the outer widget size if auto zoom is enabled
  void resizeToOuter(void);

  /// Stop search and wait for it to finish
  bool finish(void);

Q_SIGNALS:

  void scaleChanged(int);
  /// The auto-zoom state was changed
  void autoZoomChanged(bool);
  /// Context menu triggered
  void contextMenu(QContextMenuEvent*);
  /// Status bar update
  void statusChanged(VisualNode*,const Statistics&, bool);
  /// Signals that a solution has been found
  void solution(int);
  /// Signals that %Gist is finished
  void searchFinished(void);
  /// Signals that a bookmark has been added
  void addedBookmark(const QString& id);
  /// Signals that a bookmark has been removed
  void removedBookmark(int idx);
protected:
  /// Mutex for synchronizing acccess to the tree
  QMutex mutex;
  /// Mutex for synchronizing layout and drawing
  QMutex layoutMutex;
  /// Search engine thread
  SearcherThread searcher;
  /// Flag signalling the search to stop
  bool stopSearchFlag;
  /// Flag signalling that Gist is ready to be closed
  bool finishedFlag;
  /// Allocator for nodes
  Node::NodeAllocator* na;
  /// The root node of the tree
  VisualNode* root;
  /// The currently selected node
  VisualNode* currentNode;
  /// The head of the currently selected path
  VisualNode* pathHead;
//  /// The registered click inspectors, and whether they are active
//  QVector<QPair<Inspector*,bool> > doubleClickInspectors;
//  /// The registered solution inspectors, and whether they are active
//  QVector<QPair<Inspector*,bool> > solutionInspectors;
//  /// The registered move inspectors, and whether they are active
//  QVector<QPair<Inspector*,bool> > moveInspectors;
//  /// The registered comparators, and whether they are active
//  QVector<QPair<Comparator*,bool> > comparators;

  /// The bookmarks map
  QVector<VisualNode*> bookmarks;

  /// Whether node comparison action is running
  bool compareNodes;
  /// Whether node comparison action computes fixpoint
  bool compareNodesBeforeFP;

  /// The scale bar
  QSlider* scaleBar;

  /// Statistics about the search tree
  Statistics stats;

  /// Current scale factor
  double scale;
  /// Offset on the x axis so that the tree is centered
  int xtrans;

  /// Whether to hide failed subtrees automatically
  bool autoHideFailed;
  /// Whether to zoom automatically
  bool autoZoom;
  /// Whether to show copies in the tree
  bool showCopies;
  /// Refresh rate
  int refresh;
  /// Time (in msec) to pause after each refresh
  int refreshPause;
  /// Whether to use smooth scrolling and zooming
  bool smoothScrollAndZoom;
  /// Whether to move cursor during search
  bool moveDuringSearch;

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

  /// Timer for smooth zooming
  QTimeLine zoomTimeLine;
  /// Timer for smooth scrolling
  QTimeLine scrollTimeLine;
  /// Target x coordinate after smooth scrolling
  int targetX;
  /// Source x coordinate after smooth scrolling
  int sourceX;
  /// Target y coordinate after smooth scrolling
  int targetY;
  /// Target y coordinate after smooth scrolling
  int sourceY;

  /// Target width after layout
  int targetW;
  /// Target height after layout
  int targetH;
  /// Target scale after layout
  int targetScale;
  /// Timer id for delaying the update
  int layoutDoneTimerId;

  /// Timer invoked for smooth zooming and scrolling
  virtual void timerEvent(QTimerEvent* e);

  /// Similar shapes dialog
  SimilarShapesWindow shapesWindow;
  // Node that represents the shape currently selected
  VisualNode* shapeHighlighted;
public:
  std::multiset<int> tempset;
  /// Map of nodes for analyzing
  std::multiset<ShapeI, CompareShapes> shapesMap;

public Q_SLOTS:
  /// Update display
  void update(void);
  /// React to scroll events
  void scroll(void);
  /// Layout done
  void layoutDone(int w, int h, int scale0);
  /// Set the selected node to \a n
  void setCurrentNode(VisualNode* n, bool finished=true, bool update=true);
private Q_SLOTS:
  /// Search has finished
  void statusChanged(bool);
  /// Export PDF of the subtree of \a n
  void exportNodePDF(VisualNode* n);
  /// Scroll to \a i percent of the target
  void scroll(int i);
};

#endif // TREECANVAS_HH

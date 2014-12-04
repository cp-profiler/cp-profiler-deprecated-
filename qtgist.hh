#ifndef GIST_HH
#define GIST_HH

#include "treecanvas.hh"
// #include "treebuilder.hh"
#include "treecomparison.hh"
#include "nodestats.hh"
#include "recieverthread.hh"
#include "treedialog.hh"

/**
 * \brief %Gecode Interactive %Search Tool
 *
 * This class provides an interactive search tree viewer and explorer as
 * a Qt widget. You can embedd or inherit from this widget to use %Gist
 * in your own project.
 *
 * \ingroup TaskGist
 */

class Gist : public QWidget {
  Q_OBJECT

  friend RecieverThread;

  /// **************** INTERFACE *******************

  QAbstractScrollArea* scrollArea;
  QPalette* myPalette;
  QGridLayout* layout;


  /// this should replace most of current GIST!
  TreeDialog* _td;


  /// **************** MY STUFF ********************
private:

  /// Checks for new nodes
  RecieverThread* reciever;

  TreeCanvas* current_tc;
  
  /// The canvas implementation
  TreeCanvas* canvas;

  /// Second canvas in case of comparing
  TreeCanvas* canvasTwo;

  QDialog* canvasDialog;

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
  /// Action used when no solution inspector is registered
  QAction* nullSolutionInspector;
  /// Menu of solution inspectors
  QMenu* solutionInspectorMenu;
  /// Action used when no double click inspector is registered
  QAction* nullDoubleClickInspector;
  /// Menu of double click inspectors
  QMenu* doubleClickInspectorMenu;
  /// Action used when no double click inspector is registered
  QAction* nullMoveInspector;
  /// Menu of double click inspectors
  QMenu* moveInspectorMenu;
  /// Action used when no comparator is registered
  QAction* nullComparator;
  /// Menu of comparators
  QMenu* comparatorMenu;
  /// Action used when no bookmark exists
  QAction* nullBookmark;
  /// Bookmark menu
  QMenu* bookmarksMenu;
  /// Menu for direct node inspection
  QMenu* inspectNodeMenu;
  /// Menu for direct node inspection before fixpoint
  QMenu* inspectNodeBeforeFPMenu;
  /// Information about individual nodes
  NodeStatInspector* nodeStatInspector;
public:
  /// Inspect current node
  QAction* inspect;
  /// Inspect current node before fixpoint
  QAction* inspectBeforeFP;
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
  /// Unhide all hidden subtrees under current node
  QAction* unhideAll;
  /// Label branches under current node
  QAction* labelBranches;
  /// Label branches on path to root
  QAction* labelPath;
  /// Analyze similar subtrees
  QAction* analyzeSimilarSubtrees;
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
  /// Creates new canvas and runs comparison tool
  QAction* initComparison;
  /// Allow second canvas (when new data recieved)
  QAction* sndCanvas;

  /// Bookmark current node
  QAction* bookmarkNode;
  /// Compare current node to other node
  QAction* compareNode;
  /// Compare current node to other node before fixpoint
  QAction* compareNodeBeforeFP;
  /// Set path from current node to the root
  QAction* setPath;
  /// Inspect all nodes on selected path
  QAction* inspectPath;
  /// Open node statistics inspector
  QAction* showNodeStats;
  /// Bookmark current node
  QAction* toggleStop;
  /// Bookmark current node
  QAction* unstopAll;


  /// Group of all actions for solution inspectors
  QActionGroup* solutionInspectorGroup;
  /// Group of all actions for double click inspectors
  QActionGroup* doubleClickInspectorGroup;
  /// Group of all actions for move inspectors
  QActionGroup* moveInspectorGroup;
  /// Group of all actions for comparators
  QActionGroup* comparatorGroup;
  /// Group of all actions for bookmarks
  QActionGroup* bookmarksGroup;
  /// Group of all actions for direct inspector selection
  QActionGroup* inspectGroup;
  /// Group of all actions for direct inspector selection
  QActionGroup* inspectBeforeFPGroup;

public:
  /// Constructor
//  Gist(Space* root, bool bab, QWidget* parent, const Options& opt);
  Gist(QWidget* parent);
  /// Destructor
  ~Gist(void);

//  /// Add double click inspector \a i0
//  void addDoubleClickInspector(Inspector* i0);
//  /// Add solution inspector \a i0
//  void addSolutionInspector(Inspector* i0);
//  /// Add move inspector \a i0
//  void addMoveInspector(Inspector* i0);
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


  /// ***** GETTERS *****
  TreeCanvas* getCanvas(void) { return canvas; }

Q_SIGNALS:

  

  /// Signals that the tree has changed
  void statusChanged(const Statistics&, bool);

  /// Signals that a solution has been found
  void solution(int);


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
//  /// Reacts on double click inspector selection
//  void selectDoubleClickInspector(QAction*);
//  /// Reacts on solution inspector selection
//  void selectSolutionInspector(QAction*);
//  /// Reacts on move inspector selection
//  void selectMoveInspector(QAction*);
//  /// Reacts on comparator selection
//  void selectComparator(QAction*);
  /// Reacts on bookmark selection
  void selectBookmark(QAction*);
  /// Reacts on adding a bookmark
  void addBookmark(const QString& id);
  /// Reacts on removing a bookmark
  void removeBookmark(int idx);
  /// Populate the inspector menus from the actions found in Gist
  void populateInspectors(void);
  /// Populate the bookmarks menu
  void populateBookmarksMenu(void);
  /// Shows node status information
  void showStats(void);
  /// Inspect current node with inspector described by \a a
  void inspectWithAction(QAction* a);
  /// Inspect current node with inspector described by \a a
  void inspectBeforeFPWithAction(QAction* a);

public Q_SLOTS:
  void onFocusChanged(QWidget*, QWidget*);

protected:
  /// Add inspector \a i0
//  void addInspector(Inspector* i, QAction*& nas, QAction*& nad,
//                    QAction*& nam);
};


#endif // GIST_HH

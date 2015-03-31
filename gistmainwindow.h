#ifndef GISTMAINWINDOW_H
#define GISTMAINWINDOW_H

#include "qtgist.hh"

#include <QMainWindow>

/// Display information about %Gist
class AboutGist : public QDialog {
public:
  /// Constructor
  AboutGist(QWidget* parent = 0);
};

/**
 * \brief Main window for stand-alone %Gist
 *
 * \ingroup TaskGist
 */
class GistMainWindow : public QMainWindow {
  Q_OBJECT
private:
  /// Whether search is currently running
  bool isSearching;
//  /// Measure search time
//  Support::Timer searchTimer;
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
  /// Menu for solution inspectors
  QMenu* solutionInspectorsMenu;
  /// Menu for double click inspectors
  QMenu* doubleClickInspectorsMenu;
  /// Menu for move inspectors
  QMenu* moveInspectorsMenu;
  /// Menu for comparators
  QMenu* comparatorsMenu;
  /// Menu for bookmarks
  QMenu* bookmarksMenu;
  /// Menu for direct node inspection
  QMenu* inspectNodeMenu;
  /// Menu for direct node inspection before fixpoint
  QMenu* inspectNodeBeforeFPMenu;
  /// Action for activating the preferences menu
  QAction* prefAction;
protected:
  /// The contained %Gist object
  Gist* c;
  /// A menu bar
  QMenuBar* menuBar;
  /// About dialog
  AboutGist aboutGist;

Q_SIGNALS:

  /// stops the receiver thread
  void stopReceiver(void);

protected Q_SLOTS:
  /// The status has changed (e.g., new solutions have been found)
  void statusChanged(const Statistics& stats, bool finished);
  /// Open the about dialog
  void about(void);
  /// Open the preferences dialog
  void preferences(bool setup=false);
  /// Populate the inspector menus from the actions found in Gist
  void populateInspectorSelection(void);
  /// Populate the inspector menus from the actions found in Gist
  void populateInspectors(void);
  /// Populate the bookmarks menus from the actions found in Gist
  void populateBookmarks(void);
public:
  /// Constructor
  GistMainWindow(void);

  Gist* getGist(void) { return c; }
protected:
  /// Close Gist
  void closeEvent(QCloseEvent* event);
};

#endif // MAINWINDOW_H

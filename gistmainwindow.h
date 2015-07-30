/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
  /// Menu for bookmarks
  QMenu* bookmarksMenu;
  /// Action for activating the preferences menu
  QAction* prefAction;
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
  /// Populate the bookmarks menus from the actions found in Gist
  void populateBookmarks(void);
  /// Change MainWindow's title to fzn file name
  void changeTitle(const char* file_name);
public:
  /// Constructor
  GistMainWindow();

  Gist* getGist(void) { return c; }
protected:
  /// Close Gist
  void closeEvent(QCloseEvent* event);
};

#endif // MAINWINDOW_H

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
  Gist* m_Gist;
  /// A menu bar
  QMenuBar* menuBar;
  /// About dialog
  AboutGist aboutGist;

  QString statsFilename;

Q_SIGNALS:

  /// stops the receiver thread
  void stopReceiver(void);

  void doneReceiving(void);

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
public Q_SLOTS:
  void changeTitle(QString file_name);
  void gatherStatistics(void);

  void selectNode(int gid);
  void selectManyNodes(QVariantList gids);
public:
  /// Constructor
  GistMainWindow(Execution* execution, QWidget* parent);

  Gist* getGist(void) { return m_Gist; }

  void setStatsFilename(QString filename) {
      statsFilename = filename;
  }
protected:
  /// Close Gist
  /* void closeEvent(QCloseEvent* event); */
};

#endif // MAINWINDOW_H

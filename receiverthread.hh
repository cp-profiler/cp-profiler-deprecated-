/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef receiver_THREAD_HH
#define receiver_THREAD_HH

#include <QThread>

#include "qtgist.hh"
#include "treecanvas.hh"

class Gist;

class ReceiverThread : public QThread {
  Q_OBJECT

  friend Gist;

public:

  ReceiverThread(QWidget* parent = 0);
  void switchCanvas(TreeCanvas* tc);
  void receive(TreeCanvas* tc);


private:
  TreeCanvas* _t;
  Gist* ptr_gist;

  volatile bool _quit;

public Q_SLOTS:
  void updateCanvas(void); /// TODO: should be moved to TreeCanvas
  void stopThread(void);

Q_SIGNALS:
  void update(int w, int h, int scale0);
  void startReceiving(void);
  void doneReceiving(void);
  void statusChanged(bool);
  void newCanvasNeeded(void);
  /// Emit when receives new nodes to update Status Bar
  void receivedNodes(bool finished);

protected:
  void run(void);
  


};

#endif



/// ***************************************
/// some of the stuff I got rid of for now:

// private:
// int depth
// bool a

// Q_SIGNALS:
// void scaleChanged(int);     // seems like scale works even without int
// void solution(void);        // got rid of since don't have Space
// void searchFinished(void);  // dont have the function in TreeCanvas anymore
// void moveToNode(VisualNode* n, bool); // 

/// **************************************

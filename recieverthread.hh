#ifndef RECIEVER_THREAD_HH
#define RECIEVER_THREAD_HH

#include <QThread>

#include "qtgist.hh"
#include "treecanvas.hh"

class Gist;

class RecieverThread : public QThread {
  Q_OBJECT

  friend Gist;

public:

  RecieverThread(QWidget* parent = 0);


private:
  TreeCanvas* t;
  Gist* ptr_gist;

public Q_SLOTS:
  void updateCanvas(void);

Q_SIGNALS:
  void update(int w, int h, int scale0);
  void startWork(void);
  void statusChanged(bool);
  void newCanvasNeeded(void);

protected:
  void run(void);
  void recieve(TreeCanvas* tc);


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

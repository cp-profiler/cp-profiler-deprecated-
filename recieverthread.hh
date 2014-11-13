#ifndef RECIEVER_THREAD_HH
#define RECIEVER_THREAD_HH

#include <QThread>

#include "treecanvas.hh"

class Gist;

class RecieverThread : public QThread {
  Q_OBJECT

  friend Gist;

private:
  TreeCanvas* t;
  VisualNode* _node; /// TODO: delete if not used

public Q_SLOTS:
  void updateCanvas(void);

Q_SIGNALS:
  void update(int w, int h, int scale0);
  void startWork(void);
  void statusChanged(bool);

protected:
  void run(void);
  void recieve(TreeCanvas* tc);


};

#endif
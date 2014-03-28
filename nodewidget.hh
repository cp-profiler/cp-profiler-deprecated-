#ifndef NODEWIDGET_HH
#define NODEWIDGET_HH

#include "qtgist.hh"

/// \brief Small node drawings for the status bar
class NodeWidget : public QWidget {
public:
  NodeWidget(NodeStatus s);
protected:
  NodeStatus status;
  void paintEvent(QPaintEvent*);
};

#endif // NODEWIDGET_HH

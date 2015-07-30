/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

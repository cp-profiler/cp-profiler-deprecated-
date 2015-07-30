/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nodewidget.hh"
#include "drawingcursor.hh"

const double NODE_WIDTH = 20.0;
const double HALF_NODE_WIDTH = NODE_WIDTH / 2.0;
const double THIRD_NODE_WIDTH = NODE_WIDTH / 3.0;

NodeWidget::NodeWidget(NodeStatus s) : status(s) {
    setMinimumSize(22,22);
    setMaximumSize(22,22);
}

void NodeWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int hw= width()/2;
    int myx = hw+2; int myy = 2;
    switch (status) {
    case SOLVED:
    {
        QPoint points[4] = {QPoint(myx,myy),
                            QPoint(myx+8,myy+8),
                            QPoint(myx,myy+16),
                            QPoint(myx-8,myy+8)
                           };
        painter.setBrush(QBrush(DrawingCursor::green));
        painter.drawConvexPolygon(points, 4);
    }
        break;
    case FAILED:
    {
        painter.setBrush(QBrush(DrawingCursor::red));
        painter.drawRect(myx-6, myy+2, 12, 12);
    }
        break;
    case BRANCH:
    {
        painter.setBrush(QBrush(DrawingCursor::blue));
        painter.drawEllipse(myx-8, myy, 16, 16);
    }
        break;
    case UNDETERMINED:
    {
        painter.setBrush(QBrush(Qt::white));
        painter.drawEllipse(myx-8, myy, 16, 16);
    }
        break;
    case MERGING: {

        painter.setBrush(DrawingCursor::orange);

        QPointF points[5] = { QPointF(myx, myy),
            QPointF(myx + HALF_NODE_WIDTH, myy + THIRD_NODE_WIDTH),
            QPointF(myx + THIRD_NODE_WIDTH, myy + NODE_WIDTH),
            QPointF(myx - THIRD_NODE_WIDTH, myy + NODE_WIDTH),
            QPointF(myx - HALF_NODE_WIDTH, myy + THIRD_NODE_WIDTH)
        };

        painter.drawConvexPolygon(points, 5);

    }
        break;
    default:
        break;
    }
}


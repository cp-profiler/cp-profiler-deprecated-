#include "nodewidget.hh"
#include "drawingcursor.hh"

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
    default:
        break;
    }
}


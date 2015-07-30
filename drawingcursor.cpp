/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "drawingcursor.hh"

/// Red color for failed nodes
const QColor DrawingCursor::red(218, 37, 29);
/// Green color for solved nodes
const QColor DrawingCursor::green(11, 118, 70);
/// Blue color for choice nodes
const QColor DrawingCursor::blue(0, 92, 161);
/// Orange color for best solutions
const QColor DrawingCursor::orange(235, 137, 27);
/// White color
const QColor DrawingCursor::white(255,255,255);

/// Red color for expanded failed nodes
const QColor DrawingCursor::lightRed(218, 37, 29, 120);
/// Green color for expanded solved nodes
const QColor DrawingCursor::lightGreen(11, 118, 70, 120);
/// Blue color for expanded choice nodes
const QColor DrawingCursor::lightBlue(0, 92, 161, 120);

const double NODE_WIDTH = 20.0;
const double HALF_NODE_WIDTH = NODE_WIDTH / 2.0;
const double THIRD_NODE_WIDTH = NODE_WIDTH / 3.0;
const double TWO_THIRDS_NODE_WIDTH = 2.0 * NODE_WIDTH / 3.0;
const double QUARTER_NODE_WIDTH = HALF_NODE_WIDTH / 2.0;
const double FAILED_WIDTH = 14.0;
const double HALF_FAILED_WIDTH = FAILED_WIDTH / 2.0;
const double QUARTER_FAILED_WIDTH = FAILED_WIDTH / 4.0;
const double SHADOW_OFFSET = 3.0;
const double HIDDEN_DEPTH =
  static_cast<double>(Layout::dist_y) + FAILED_WIDTH;

DrawingCursor::DrawingCursor(VisualNode* root,
                             const VisualNode::NodeAllocator& na,
                             QPainter& painter0,
                             const QRect& clippingRect0,
                             bool showHidden)
    : NodeCursor<VisualNode>(root,na), painter(painter0), 
      clippingRect(clippingRect0), x(0.0), y(0.0),
      _showHidden(showHidden)
{
    QPen pen = painter.pen();
    pen.setWidth(1);
    painter.setPen(pen);
}

void
DrawingCursor::processCurrentNode(void) {
    VisualNode* n = node();
    VisualNode* parent = n->getParent(na);
    double parentX = x - static_cast<double>(n->getOffset());
    double parentY = y - static_cast<double>(Layout::dist_y) + NODE_WIDTH;
    if ( !n->isRoot() && (parent->getStatus() == STOP || parent->getStatus() == UNSTOP) )
        parentY -= (NODE_WIDTH - FAILED_WIDTH) / 2;

    double myx = x;
    double myy = y;

    if (n->getStatus() == STOP || n->getStatus() == UNSTOP)
        myy += (NODE_WIDTH - FAILED_WIDTH) / 2;

    if (n != startNode()) {
        if (n->isOnPath())
            painter.setPen(Qt::red);
        else
            painter.setPen(Qt::black);
        // Here we use drawPath instead of drawLine in order to
        // workaround a strange redraw artefact on Windows
        QPainterPath path;
        path.moveTo(myx,myy);
        path.lineTo(parentX,parentY);
        painter.drawPath(path);

        QFontMetrics fm = painter.fontMetrics();
        QString label = na.getLabel(n);
        int alt = n->getAlternative(na);
        int n_alt = parent->getNumberOfChildren();
        int tw = fm.width(label);
        int lx;
        if (alt == 0 && n_alt > 1) {
            lx = myx - tw - 4;
        } else if (alt == n_alt - 1 && n_alt > 1) {
            lx = myx + 4;
        } else {
            lx = myx - tw / 2;
        }
        painter.drawText(QPointF(lx, myy - 2), label);
    }

    if (!parent || parent->_tid != n->_tid) {
        switch (n->_tid) {
            case 0:
                painter.setBrush(QColor(255, 255, 255, 255));
            break;
            case 1:
                painter.setBrush(QColor(150, 255, 255, 255));
            break;
            case 2:
                painter.setBrush(QColor(255, 150, 255, 255));
            break;
            case 3:
                painter.setBrush(QColor(150, 255, 150, 255));
            break;
            default:
                painter.setBrush(QColor(200, 200, 200, 255));
        }
        drawShape(myx, myy, n);
    }

    // draw the shape if the node is highlighted
    painter.setBrush(QColor(160, 160, 160, 125));
    if (n->isHighlighted()) {
      drawShape(myx, myy, n);
    }

    // draw shadow
    if (n->isMarked()) {
        painter.setBrush(Qt::gray);
        painter.setPen(Qt::NoPen);
        if (n->isHidden()) {
            if (n->getStatus() == MERGING)
                drawPentagon(myx, myy, true);
            else
              if (n->getSubtreeSize() != -1)
                drawSizedTriangle(myx, myy, n->getSubtreeSize(), true);
              else
                drawTriangle(myx, myy, true);
        } else {
            switch (n->getStatus()) {
            case SOLVED:
                drawDiamond(myx, myy, true);
                break;
            case FAILED:
                painter.drawRect(myx - HALF_FAILED_WIDTH + SHADOW_OFFSET,
                    myy + SHADOW_OFFSET, FAILED_WIDTH, FAILED_WIDTH);
                break;
            case UNSTOP:
            case STOP:
                drawOctagon(myx, myy, false);
                break;
            case BRANCH:
                painter.drawEllipse(myx - HALF_NODE_WIDTH + SHADOW_OFFSET,
                    myy + SHADOW_OFFSET, NODE_WIDTH, NODE_WIDTH);
                break;
            case UNDETERMINED:
                painter.drawEllipse(myx - HALF_NODE_WIDTH+SHADOW_OFFSET,
                    myy + SHADOW_OFFSET, NODE_WIDTH, NODE_WIDTH);
                break;
            case SKIPPED:
                painter.drawRect(myx - HALF_FAILED_WIDTH + SHADOW_OFFSET,
                    myy + SHADOW_OFFSET, FAILED_WIDTH, FAILED_WIDTH);
                break;
            case MERGING:
                break; /// already handled, here to avoid warnings
            }
        }
    }

    painter.setPen(Qt::SolidLine);
    if (n->isHidden() && ~_showHidden) {

        if (n->getStatus() == MERGING) {
            painter.setBrush(orange);
            drawPentagon(myx, myy, false);
        } else {
            if (n->hasOpenChildren()) {
            QLinearGradient gradient(myx - NODE_WIDTH, myy,
                myx + NODE_WIDTH * 1.3, myy + HIDDEN_DEPTH * 1.3);
            if (n->hasSolvedChildren()) {
                gradient.setColorAt(0, white);
                gradient.setColorAt(1, green);
            } else if (n->hasFailedChildren()) {
                gradient.setColorAt(0, white);
                gradient.setColorAt(1, red);
            } else {
                gradient.setColorAt(0, white);
                gradient.setColorAt(1, QColor(0, 0, 0));
            }
            painter.setBrush(gradient);
            } else {
                if (n->hasSolvedChildren())
                    painter.setBrush(QBrush(green));
                else
                    painter.setBrush(QBrush(red));
            }

            if (n->getSubtreeSize() != -1)
              drawSizedTriangle(myx, myy, n->getSubtreeSize(), false);
            else
              drawTriangle(myx, myy, false);
        }
        
    } else {
        switch (n->getStatus()) {
        case SOLVED:
            // if (n->isCurrentBest(curBest)) {
                // painter.setBrush(QBrush(orange));
            // } else {
                painter.setBrush(QBrush(green));
            // }
            drawDiamond(myx, myy, false);
            break;
        case FAILED:
            painter.setBrush(QBrush(red));
            painter.drawRect(myx - HALF_FAILED_WIDTH, myy, FAILED_WIDTH, FAILED_WIDTH);
            break;
        case UNSTOP:
        case STOP:
            painter.setBrush(n->getStatus() == STOP ? QBrush(red) : QBrush(green));
            drawOctagon(myx, myy, false);
            break;
        case BRANCH:
            painter.setBrush(n->childrenLayoutIsDone() ? QBrush(blue) :
                                                         QBrush(white));
            painter.drawEllipse(myx - HALF_NODE_WIDTH, myy, NODE_WIDTH, NODE_WIDTH);
            break;
        case UNDETERMINED:
            painter.setBrush(Qt::white);
            painter.drawEllipse(myx - HALF_NODE_WIDTH, myy, NODE_WIDTH, NODE_WIDTH);
            break;
        case SKIPPED:
            painter.setBrush(Qt::gray);
            painter.drawRect(myx - HALF_FAILED_WIDTH, myy, FAILED_WIDTH, FAILED_WIDTH);
            break;
        case MERGING:
            painter.setBrush(orange);
            drawPentagon(myx, myy, false);
            break;
        }
    }

    if (n->isBookmarked()) {
        painter.setBrush(Qt::black);
        painter.drawEllipse(myx-10-0, myy, 10.0, 10.0);
    }

}

inline void
DrawingCursor::drawPentagon(int myx, int myy, bool shadow) {
    int shadowOffset = shadow? SHADOW_OFFSET : 0;
    QPointF points[5] = { QPointF(myx + shadowOffset, myy + shadowOffset),
        QPointF(myx + HALF_NODE_WIDTH + shadowOffset, myy + THIRD_NODE_WIDTH + shadowOffset),
        QPointF(myx + THIRD_NODE_WIDTH + shadowOffset, myy + NODE_WIDTH + shadowOffset),
        QPointF(myx - THIRD_NODE_WIDTH + shadowOffset, myy + NODE_WIDTH + shadowOffset),
        QPointF(myx - HALF_NODE_WIDTH + shadowOffset, myy + THIRD_NODE_WIDTH + shadowOffset)
    };

    painter.drawConvexPolygon(points, 5);
}

inline void
DrawingCursor::drawTriangle(int myx, int myy, bool shadow){
    int shadowOffset = shadow? SHADOW_OFFSET : 0;
    QPointF points[3] = { QPointF(myx + shadowOffset, myy + shadowOffset),
        QPointF(myx + NODE_WIDTH + shadowOffset, myy + HIDDEN_DEPTH + shadowOffset),
        QPointF(myx - NODE_WIDTH + shadowOffset, myy + HIDDEN_DEPTH + shadowOffset)
    };

    painter.drawConvexPolygon(points, 3);
}

inline void
  DrawingCursor::drawSizedTriangle(int myx, int myy, int subtreeSize, bool shadow){
    int shadowOffset = shadow? SHADOW_OFFSET : 0;
    int height = HIDDEN_DEPTH * (subtreeSize + 1) / 4;
    QPointF points[3] = { QPointF(myx + shadowOffset, myy + shadowOffset),
        QPointF(myx + NODE_WIDTH + shadowOffset, myy + height + shadowOffset),
        QPointF(myx - NODE_WIDTH + shadowOffset, myy + height + shadowOffset)
    };

    painter.drawConvexPolygon(points, 3);
}

inline void
DrawingCursor::drawDiamond(int myx, int myy, bool shadow){
    int shadowOffset = shadow? SHADOW_OFFSET : 0;
    QPointF points[4] = { QPointF(myx + shadowOffset, myy + shadowOffset),
        QPointF(myx + HALF_NODE_WIDTH + shadowOffset, myy + HALF_NODE_WIDTH + shadowOffset),
        QPointF(myx + shadowOffset, myy + NODE_WIDTH + shadowOffset),
        QPointF(myx - HALF_NODE_WIDTH + shadowOffset, myy + HALF_NODE_WIDTH + shadowOffset)
    };

painter.drawConvexPolygon(points, 4);
}

inline void
DrawingCursor::drawOctagon(int myx, int myy, bool shadow){
    int so = shadow? SHADOW_OFFSET : 0;

    QPointF points[8] = {
        QPointF(myx - QUARTER_FAILED_WIDTH + so, myy  + so), 
        QPointF(myx + QUARTER_FAILED_WIDTH  + so, myy  + so),
        QPointF(myx + HALF_FAILED_WIDTH  + so, myy + QUARTER_FAILED_WIDTH + so),
        QPointF(myx + HALF_FAILED_WIDTH + so, myy + HALF_FAILED_WIDTH + QUARTER_FAILED_WIDTH + so),
        QPointF(myx + QUARTER_FAILED_WIDTH + so, myy + FAILED_WIDTH + so),
        QPointF(myx - QUARTER_FAILED_WIDTH + so, myy + FAILED_WIDTH + so),
        QPointF(myx - HALF_FAILED_WIDTH + so, myy + HALF_FAILED_WIDTH + QUARTER_FAILED_WIDTH + so),
        QPointF(myx - HALF_FAILED_WIDTH + so, myy + QUARTER_FAILED_WIDTH + so)
    };

    painter.drawConvexPolygon(points, 8);
}

inline void 
DrawingCursor::drawShape(int myx, int myy, VisualNode* node){
    painter.setPen(Qt::NoPen);

    Shape* shape = node->getShape();
    int depth = shape->depth();
    QPointF *points = new QPointF[depth * 2];

    int l_x = myx + (*shape)[0].l;
    int r_x = myx + (*shape)[0].r;
    int y = myy + NODE_WIDTH / 2;

    points[0] = QPointF(l_x, myy);
    points[depth * 2 - 1] = QPointF(r_x, myy);

    for (int i = 1; i <  depth; i++){
        y += static_cast<double>(Layout::dist_y);
        l_x += (*shape)[i].l;
        r_x += (*shape)[i].r;
        points[i] = QPointF(l_x, y);
        points[depth * 2 - i - 1] = QPointF(r_x, y);  
    }    

    painter.drawConvexPolygon(points, shape->depth() * 2);
    
    delete[] points;

    // delete[] shape;
}


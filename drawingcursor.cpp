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

#include "drawingcursor.hh"

using namespace cpprofiler::colors;

constexpr double NODE_WIDTH = 20.0;
constexpr double HALF_NODE_WIDTH = NODE_WIDTH / 2.0;
constexpr double THIRD_NODE_WIDTH = NODE_WIDTH / 3.0;
constexpr double FAILED_WIDTH = 14.0;
constexpr double HALF_FAILED_WIDTH = FAILED_WIDTH / 2.0;
constexpr double QUARTER_FAILED_WIDTH = FAILED_WIDTH / 4.0;
constexpr double SHADOW_OFFSET = 3.0;
constexpr double HIDDEN_DEPTH =
    static_cast<double>(Layout::dist_y) + FAILED_WIDTH;


namespace Pens {
    const QPen hovered = QPen{Qt::black, 3};
}

static void drawPentagon(QPainter& painter, int myx, int myy, bool shadow);
static void drawTriangle(QPainter& painter, int myx, int myy, bool shadow);
static void drawDiamond(QPainter& painter, int myx, int myy, bool shadow);
static void drawShape(QPainter& painter, int myx, int myy, VisualNode* node);
static void drawSizedRect(QPainter& painter, int myx, int myy, int subtreeSize, bool shadow);
static void drawSizedTriangle(QPainter& painter, int myx, int myy, int subtreeSize, bool shadow);

DrawingCursor::DrawingCursor(VisualNode* root,
                             const NodeAllocator& na,
                             QPainter& painter0,
                             const QRect& clippingRect0)
    : NodeCursor(root,na), painter(painter0),
      clippingRect(clippingRect0), x(0.0), y(0.0)
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

    double myx = x;
    double myy = y;

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

    }

    QFontMetrics fm = painter.fontMetrics();
    QString label = na.getLabel(n);
    int alt = n->getAlternative(na);
    int n_alt = parent ? parent->getNumberOfChildren() : 1;
    int tw = fm.width(label);
    int lx;
    if (alt == 0 && n_alt > 1) {
        lx = myx - tw - 4;
    } else if (alt == n_alt - 1 && n_alt > 1) {
        lx = myx + 4;
    } else {
        lx = myx - tw / 2;
    }

    // QFont font(painter.font());
    // font.setPixelSize(28);
    // painter.setFont(font);

        painter.drawText(QPointF(lx, myy - 4), label);
    // painter.drawText(QPointF(lx-5, myy), label);

    if (!parent || parent->_tid != n->_tid) {
        switch (n->_tid) {
            case 0:
                painter.setBrush(QColor(255, 255, 255, 255));
            break;
            case 1:
                painter.setBrush(QColor(150, 255, 255, 255));
            break;
            case 2:
                painter.setBrush(QColor(255, 206, 153, 255));
            break;
            case 3:
                painter.setBrush(QColor(150, 255, 150, 255));
            break;
            default:
                painter.setBrush(QColor(255, 255, 255, 255));
        }
        drawShape(painter, myx, myy, n);
    }

    // draw the shape if the node is highlighted
    painter.setBrush(QColor(160, 160, 160, 100));
    if (n->isHighlighted()) {
      drawShape(painter, myx, myy, n);
    }

    if (n->isInvisible()) return;

    // draw as currently selected
    if (n->isMarked()) {
        painter.setBrush(Qt::gray);
        painter.setPen(Qt::NoPen);
        if (n->isHidden()) {
            if (n->getStatus() == MERGING)
                drawPentagon(painter, myx, myy, true);
            else
              if (n->getSubtreeSize() != -1) {
                drawSizedRect(painter, myx, myy, n->getSubtreeSize(), true);
            }
              else {
                drawTriangle(painter, myx, myy, true);
              }
        } else {
            switch (n->getStatus()) {
            case SOLVED:
                drawDiamond(painter, myx, myy, true);
                break;
            case FAILED:
                painter.drawRect(myx - HALF_FAILED_WIDTH + SHADOW_OFFSET,
                    myy + SHADOW_OFFSET, FAILED_WIDTH, FAILED_WIDTH);
                break;
            case BRANCH:
#ifndef MAXIM_THESIS
                painter.drawEllipse(myx - HALF_NODE_WIDTH + SHADOW_OFFSET,
                    myy + SHADOW_OFFSET, NODE_WIDTH, NODE_WIDTH);
#endif
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
            default: { break; }
            }
        }
    }

    if (n->isHovered() || n->isSelected()) {
        /// TODO(maxim): maybe make the brush color darker as well
        // pen.set
        painter.setPen(Pens::hovered);
    } else {
        painter.setPen(Qt::SolidLine);
    }

    if (n->isHidden()) {

        if (n->getStatus() == MERGING) {
            if (n->isMarked()) {
                painter.setBrush(gold);
            } else {
                painter.setBrush(orange);
            }
            drawPentagon(painter, myx, myy, false);
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
              drawSizedRect(painter, myx, myy, n->getSubtreeSize(), false);
            else
              drawTriangle(painter, myx, myy, false);
        }

    } else {
        switch (n->getStatus()) {
        case SOLVED:
            // if (n->isCurrentBest(curBest)) {
                // painter.setBrush(QBrush(orange));
            // } else {
                painter.setBrush(QBrush(green));
            // }
            drawDiamond(painter, myx, myy, false);
            break;
        case FAILED:
            if (n->isMarked())
                painter.setBrush(gold);
            else
                painter.setBrush(QBrush(red));
            painter.drawRect(myx - HALF_FAILED_WIDTH, myy, FAILED_WIDTH, FAILED_WIDTH);
            break;
        case BRANCH:
#ifndef MAXIM_THESIS
            // if (n->isMarked())
            //     painter.setBrush(gold);
            // else
#endif
                painter.setBrush(n->childrenLayoutIsDone() ? QBrush(blue) :
                                                             QBrush(white));
            painter.drawEllipse(myx - HALF_NODE_WIDTH, myy, NODE_WIDTH, NODE_WIDTH);
            break;
        case UNDETERMINED:
            if (n->isMarked())
                painter.setBrush(gold);
            else
                painter.setBrush(Qt::white);
            painter.drawEllipse(myx - HALF_NODE_WIDTH, myy, NODE_WIDTH, NODE_WIDTH);
            break;
        case SKIPPED:
            if (n->isMarked())
                painter.setBrush(gold);
            else
                painter.setBrush(Qt::gray);
            painter.drawRect(myx - HALF_FAILED_WIDTH, myy, FAILED_WIDTH, FAILED_WIDTH);
            break;
        case MERGING:
            if (n->isMarked()) {
                painter.setBrush(gold);
            } else {
                painter.setBrush(orange);
            }
            drawPentagon(painter, myx, myy, false);
            break;
        default: break;
        }
    }

    if (n->isBookmarked()) {
        painter.setBrush(Qt::black);
        painter.drawEllipse(myx-10-0, myy, 10.0, 10.0);
    }



}

static void drawPentagon(QPainter& painter, int myx, int myy, bool shadow) {

    QPointF points[5] = { QPointF(myx, myy),
        QPointF(myx + HALF_NODE_WIDTH, myy + THIRD_NODE_WIDTH),
        QPointF(myx + THIRD_NODE_WIDTH, myy + NODE_WIDTH),
        QPointF(myx - THIRD_NODE_WIDTH, myy + NODE_WIDTH),
        QPointF(myx - HALF_NODE_WIDTH, myy + THIRD_NODE_WIDTH)
    };

    if (shadow) {
        for (auto&& p : points) { p += QPointF(SHADOW_OFFSET, SHADOW_OFFSET);}
    }

    painter.drawConvexPolygon(points, 5);
}

static void drawTriangle(QPainter& painter, int myx, int myy, bool shadow){

    QPointF points[3] = { QPointF(myx, myy),
        QPointF(myx + NODE_WIDTH, myy + HIDDEN_DEPTH),
        QPointF(myx - NODE_WIDTH, myy + HIDDEN_DEPTH)
    };

    if (shadow) {
        for (auto&& p : points) { p += QPointF(SHADOW_OFFSET, SHADOW_OFFSET);}
    }

    painter.drawConvexPolygon(points, 3);
}

static void drawSizedTriangle(QPainter& painter, int myx, int myy, int subtreeSize, bool shadow) {

    int height = HIDDEN_DEPTH * (subtreeSize + 1) / 4;
    QPointF points[3] = { QPointF(myx, myy),
        QPointF(myx + NODE_WIDTH, myy + height),
        QPointF(myx - NODE_WIDTH, myy + height)
    };

    if (shadow) {
        for (auto&& p : points) { p += QPointF(SHADOW_OFFSET, SHADOW_OFFSET);}
    }

    painter.drawConvexPolygon(points, 3);
}

static void drawSizedRect(QPainter& painter, int myx, int myy, int subtreeSize, bool shadow) {
    using sized_rect::K; using sized_rect::BASE_HEIGHT; using sized_rect::HALF_WIDTH;

    int height = K * subtreeSize;

    QPointF points[5] = { QPointF(myx, myy),
        QPointF(myx + HALF_WIDTH, myy + BASE_HEIGHT),
        QPointF(myx + HALF_WIDTH, myy + BASE_HEIGHT + height),
        QPointF(myx - HALF_WIDTH, myy + BASE_HEIGHT + height),
        QPointF(myx - HALF_WIDTH, myy + BASE_HEIGHT)
    };

    if (shadow) {
        for (auto&& p : points) { p += QPointF(SHADOW_OFFSET, SHADOW_OFFSET);}
    }

    painter.drawConvexPolygon(points, 5);

    // QRectF rect { (float)myx - HALF_NODE_WIDTH, (float)myy, (float)NODE_WIDTH, (float)height };

    // if (shadow) rect.translate(SHADOW_OFFSET, SHADOW_OFFSET);

    // painter.drawRect(rect);
}

static void drawDiamond(QPainter& painter, int myx, int myy, bool shadow) {

    QPointF points[4] = { QPointF(myx, myy),
        QPointF(myx + HALF_NODE_WIDTH, myy + HALF_NODE_WIDTH),
        QPointF(myx, myy + NODE_WIDTH),
        QPointF(myx - HALF_NODE_WIDTH, myy + HALF_NODE_WIDTH)
    };

    if (shadow) {
        for (auto&& p : points) { p += QPointF(SHADOW_OFFSET, SHADOW_OFFSET);}
    }

    painter.drawConvexPolygon(points, 4);
}


static void drawShape(QPainter& painter, int myx, int myy, VisualNode* node){
    painter.setPen(Qt::NoPen);

    Shape* shape = node->getShape();
    if (shape == nullptr) {
        std::cerr << "WARNING: node has no shape\n";
        abort();
    }
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
}
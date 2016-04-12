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

#ifndef GECODE_GIST_QT_DRAWINGCURSOR_HH
#define GECODE_GIST_QT_DRAWINGCURSOR_HH

#include "nodecursor.hh"
#include "layoutcursor.hh"
#include <QtGui>

/// \brief A cursor that draws a tree on a QWidget
class DrawingCursor : public NodeCursor<VisualNode> {
private:
    /// The painter where the tree is drawn
    QPainter& painter;
    /// The clipping area
    QRect clippingRect;
    /// The current coordinates
    double x, y;

    bool _showHidden;

    /// Test if current node is clipped
    bool isClipped(void);

    void drawPentagon(int myx, int myy, bool shadow);
    void drawTriangle(int myx, int myy, bool shadow);
    void drawSizedTriangle(int myx, int myy, int size, bool shadow);
    void drawDiamond(int myx, int myy, bool shadow);
    void drawOctagon(int myx, int myy, bool shadow);
    void drawShape(int myx, int myy, VisualNode* node);
public:
    static const QColor gold;
    /// The color for failed nodes
    static const QColor red;
    /// The color for solved nodes
    static const QColor green;
    /// The color for choice nodes
    static const QColor blue;
    /// The color for the best solution
    static const QColor orange;
    /// White color
    static const QColor white;

    /// The color for expanded failed nodes
    static const QColor lightRed;
    /// The color for expanded solved nodes
    static const QColor lightGreen;
    /// The color for expanded choice nodes
    static const QColor lightBlue;

    /// Constructor
    DrawingCursor(VisualNode* root,
                  const NodeAllocator& na,
                  QPainter& painter0,
                  const QRect& clippingRect0,
                  bool showHidden = false);

    ///\name Cursor interface
    //@{
    /// Move cursor to parent
    void moveUpwards(void);
    /// Test if cursor may move to child
    bool mayMoveDownwards(void);
    /// Move cursor to child
    void moveDownwards(void);
    /// Move cursor to sibling
    void moveSidewards(void);
    /// Draw the node
    void processCurrentNode(void);
    //@}
};

#include "drawingcursor.hpp"

#endif

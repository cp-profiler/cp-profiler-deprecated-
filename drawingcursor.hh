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

namespace cpprofiler {
namespace colors {
    /// The color for selected nodes
    static QColor gold(252, 209, 22);
    /// Red color for failed nodes
    static QColor red(218, 37, 29);
    /// Green color for solved nodes
    static QColor green(11, 118, 70);
    /// Blue color for choice nodes
    static QColor blue(0, 92, 161);
    /// Orange color for best solutions
    static QColor orange(235, 137, 27);
    /// White color
    static QColor white(255,255,255);

    /// Red color for expanded failed nodes
    static QColor lightRed(218, 37, 29, 120);
    /// Green color for expanded solved nodes
    static QColor lightGreen(11, 118, 70, 120);
    /// Blue color for expanded choice nodes
    static QColor lightBlue(0, 92, 161, 120);
}
}

/// \brief A cursor that draws a tree on a QWidget
class DrawingCursor : public NodeCursor {
private:
    /// The painter where the tree is drawn
    QPainter& painter;
    /// The clipping area
    QRect clippingRect;
    /// The current coordinates
    double x, y;

    /// Test if current node is clipped
    bool isClipped(void);
public:
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
                  const QRect& clippingRect0);

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

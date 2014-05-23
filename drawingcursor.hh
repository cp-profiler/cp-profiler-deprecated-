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

    bool copies;

    /// Test if current node is clipped
    bool isClipped(void);

    void drawTriangle(int myx, int myy, bool shadow);
    void drawDiamond(int myx, int myy, bool shadow);
    void drawOctagon(int myx, int myy, bool shadow);
    void drawShape(int myx, int myy, VisualNode* node);
public:
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
                  const VisualNode::NodeAllocator& na,
                  QPainter& painter0,
                  const QRect& clippingRect0, bool showCopies);

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

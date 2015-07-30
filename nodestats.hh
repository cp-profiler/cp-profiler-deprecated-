/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NODESTATS_HH
#define NODESTATS_HH

#include "visualnode.hh"

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif


/**
   * \brief Display information about nodes
   */
class NodeStatInspector : public QWidget {
    Q_OBJECT
private:
    /// Label for node depth indicator
    QGraphicsTextItem* nodeDepthLabel;
    /// Label for subtree depth indicator
    QGraphicsTextItem* subtreeDepthLabel;
    /// Label for number of solutions
    QGraphicsTextItem* solvedLabel;
    /// Label for number of failures
    QGraphicsTextItem* failedLabel;
    /// Label for number of choices
    QGraphicsTextItem* choicesLabel;
    /// Label for number of open nodes
    QGraphicsTextItem* openLabel;
    /// Layout
    QVBoxLayout* boxLayout;
public:
    NodeStatInspector(QWidget* parent);
    /// Update display to reflect information about \a n
    void node(const VisualNode::NodeAllocator&, VisualNode* n,
              const Statistics& stat, bool finished);
public Q_SLOTS:
    /// Show this window and bring it to the front
    void showStats(void);
};

#endif // NODESTATS_HH

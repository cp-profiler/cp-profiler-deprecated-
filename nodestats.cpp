/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Guido Tack, 2006
 *
 *  Last modified:
 *     $Date$ by $Author$
 *     $Revision$
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "nodestats.hh"
#include "nodewidget.hh"
#include "nodecursor.hh"
#include "nodevisitor.hh"
#include "drawingcursor.hh"
#include "tree_utils.hh"


NodeStatInspector::NodeStatInspector(QWidget* parent, const NodeTree& nt)
    : QWidget(parent), nt(nt) {
    setWindowFlags(Qt::Tool);
    QGraphicsScene* scene = new QGraphicsScene(parent);

    scene->addEllipse(70,10,16,16,QPen(),QBrush(DrawingCursor::white));
    scene->addEllipse(70,60,16,16,QPen(),QBrush(DrawingCursor::blue));
    scene->addRect(32,100,12,12,QPen(),QBrush(DrawingCursor::red));

    QPolygonF poly;
    poly << QPointF(78,100) << QPointF(78+8,100+8)
         << QPointF(78,100+16) << QPointF(78-8,100+8);
    scene->addPolygon(poly,QPen(),QBrush(DrawingCursor::green));

    scene->addEllipse(110,100,16,16,QPen(),QBrush(DrawingCursor::white));

    QPen pen;
    pen.setStyle(Qt::DotLine);
    pen.setWidth(0);
    scene->addLine(78,26,78,60,pen);
    scene->addLine(78,76,38,100,pen);
    scene->addLine(78,76,78,100,pen);
    scene->addLine(78,76,118,100,pen);

    scene->addLine(135,10,145,10);
    scene->addLine(145,10,145,110);
    scene->addLine(145,60,135,60);
    scene->addLine(145,110,135,110);

    nodeDepthLabel = scene->addText("0");
    nodeDepthLabel->setPos(150,20);
    subtreeDepthLabel = scene->addText("0");
    subtreeDepthLabel->setPos(150,75);

    choicesLabel = scene->addText("0");
    choicesLabel->setPos(45,57);

    solvedLabel = scene->addText("0");
    solvedLabel->setPos(78-solvedLabel->document()->size().width()/2,120);
    failedLabel = scene->addText("0");
    failedLabel->setPos(30,120);
    openLabel = scene->addText("0");
    openLabel->setPos(110,120);

    QGraphicsView* view = new QGraphicsView(scene);
    view->setRenderHints(view->renderHints() | QPainter::Antialiasing);
    view->show();

    scene->setBackgroundBrush(Qt::white);

    boxLayout = new QVBoxLayout();
    boxLayout->setContentsMargins(0,0,0,0);
    boxLayout->addWidget(view);
    setLayout(boxLayout);

    setWindowTitle("Gist node statistics");
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose, false);
}

void
NodeStatInspector::node(VisualNode* n) {

    auto& na = nt.getNA();

    // if (isVisible()) {

        auto depth = tree_utils::calculateDepth(nt, *n);
        nodeDepthLabel->setPlainText(QString("%1").arg(depth));;
        StatCursor sc(n,na);
        PreorderNodeVisitor<StatCursor> pnv(sc);
        pnv.run();

        subtreeDepthLabel->setPlainText(
                    QString("%1").arg(pnv.getCursor().depth));
        solvedLabel->setPlainText(QString("%1").arg(pnv.getCursor().solved));
        solvedLabel->setPos(78-solvedLabel->document()->size().width()/2,120);
        failedLabel->setPlainText(QString("%1").arg(pnv.getCursor().failed));
        failedLabel->setPos(44-failedLabel->document()->size().width(),120);
        choicesLabel->setPlainText(QString("%1").arg(pnv.getCursor().choice));
        choicesLabel->setPos(66-choicesLabel->document()->size().width(),57);
        openLabel->setPlainText(QString("%1").arg(pnv.getCursor().open));
    // }
}

void
NodeStatInspector::showStats(VisualNode* n) {
    node(n);
    show();
    activateWindow();
}

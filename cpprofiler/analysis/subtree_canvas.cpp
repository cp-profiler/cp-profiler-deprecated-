#include "subtree_canvas.hh"
#include "visualnode.hh"
#include "drawingcursor.hh"
#include "nodevisitor.hh"

#include <QAbstractScrollArea>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollBar>

namespace cpprofiler {
namespace analysis {

SubtreeCanvas::SubtreeCanvas(QAbstractScrollArea* sa, const NodeTree& nt)
    : QWidget{sa}, m_ScrollArea{sa}, m_NodeTree{nt} {}

SubtreeCanvas::~SubtreeCanvas() = default;

void SubtreeCanvas::paintEvent(QPaintEvent* event) {
  /// TODO(maxim): make a copy of a subtree to display here
  /// (so that it is never hidden)
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  if (!cur_node) return;

  const int view_w = m_ScrollArea->viewport()->width();
  const int view_h = m_ScrollArea->viewport()->height();

  int xoff = m_ScrollArea->horizontalScrollBar()->value();
  int yoff = m_ScrollArea->verticalScrollBar()->value();

  const BoundingBox bb = cur_node->getBoundingBox();

  const int w = bb.right - bb.left + Layout::extent;
  const int h =
      2 * Layout::extent + cur_node->getShape()->depth() * Layout::dist_y;

  // center the shape if small
  if (w < view_w) {
    xoff -= (view_w - w) / 2;
  }

  setFixedSize(view_w, view_h);

  int xtrans = -bb.left + (Layout::extent / 2);

  m_ScrollArea->horizontalScrollBar()->setRange(0, w - view_w);
  m_ScrollArea->verticalScrollBar()->setRange(0, h - view_h);
  m_ScrollArea->horizontalScrollBar()->setPageStep(view_w);
  m_ScrollArea->verticalScrollBar()->setPageStep(view_h);

  QRect origClip = event->rect();
  painter.translate(0, 30);
  painter.translate(xtrans - xoff, -yoff);
  QRect clip{origClip.x() - xtrans + xoff, origClip.y() + yoff,
             origClip.width(), origClip.height()};

  DrawingCursor dc{cur_node, m_NodeTree.getNA(), painter, clip, false};
  PreorderNodeVisitor<DrawingCursor>(dc).run();
}

void SubtreeCanvas::showSubtree(VisualNode* node) {
  cur_node = node;
  QWidget::update();
}

}}
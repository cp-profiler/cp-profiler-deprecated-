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


  const BoundingBox bb = cur_node->getBoundingBox();

  const int shape_w = bb.right - bb.left + Layout::extent;
  const int shape_h =
      2 * Layout::extent + cur_node->getShape()->depth() * Layout::dist_y;

  int xoff = 0;
  int yoff = 0;
  // center the shape if small
  if (shape_w < view_w) {
    xoff -= (view_w - shape_w) / 2;
  }

  auto scale = std::min((float)view_w/shape_w, (float)view_h/shape_h);
  scale = std::min(scale, 1.0f);

  painter.scale(scale, scale);

  setFixedSize(view_w, view_h);

  int xtrans = -bb.left + (Layout::extent / 2);

  QRect origClip = event->rect();

  {
    const auto top_padding = 30;
    painter.translate(0, top_padding);
  }

  painter.translate(xtrans - xoff, -yoff);
  QRect clip{origClip.x() - xtrans + xoff, origClip.y() + yoff,
             (int)(origClip.width()/scale), (int)(origClip.height()/scale)};

  DrawingCursor dc{cur_node, m_NodeTree.getNA(), painter, clip};
  PreorderNodeVisitor<DrawingCursor>(dc).run();
}

void SubtreeCanvas::showSubtree(VisualNode* node) {
  cur_node = node;
  QWidget::update();
}

}}
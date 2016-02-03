
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
 */
#include <QAbstractScrollArea>
#include <climits>
#include <functional>

#include "icicle_tree_dialog.hh"
#include "treecanvas.hh"
#include "nodevisitor.hh"

using namespace cpprofiler::pixeltree;

IcicleTreeDialog::IcicleTreeDialog(TreeCanvas* tc): QDialog(tc) {

  qDebug() << "tc in IcicleTree address: " << tc;

  this->resize(INIT_WIDTH, INIT_HEIGHT);
  this->setWindowTitle(QString::fromStdString(tc->getExecution()->getData()->getTitle()));

  scrollArea_ = new QAbstractScrollArea();

  auto layout = new QVBoxLayout();
  setLayout(layout);
  layout->addWidget(scrollArea_);

  canvas_ = new IcicleTreeCanvas(scrollArea_, tc);

  connect(this, SIGNAL(windowResized()), canvas_, SLOT(resizeCanvas()));

}

void
IcicleTreeDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  emit windowResized();
}

IcicleTreeCanvas::IcicleTreeCanvas(QAbstractScrollArea* parent, TreeCanvas* tc)
: QWidget(parent), sa_(*parent), tc_(*tc) {

  connect (sa_.horizontalScrollBar(), SIGNAL(valueChanged (int)), this, SLOT(sliderChanged(int)));

  /// TODO(maxim): find out the 'width' of the icicle tree
  icicle_image_.scaleUp();
  icicle_image_.scaleUp();
  icicle_image_.scaleUp();
}

void
IcicleTreeCanvas::paintEvent(QPaintEvent*) {
  if (icicle_image_.image() == nullptr) return;

  QPainter painter(this);

  painter.drawImage(QPoint{1,1}, *icicle_image_.image());
}

void
IcicleTreeCanvas::resizeCanvas() {

  auto sa_width = sa_.viewport()->width();
  auto sa_height =  sa_.viewport()->height();
  icicle_image_.resize(sa_width, sa_height);
  this->resize(sa_width, sa_height);
  redrawAll();
}

void
IcicleTreeCanvas::redrawAll() {
  /// TODO(maxim): meaningful value instead of 10000
  sa_.horizontalScrollBar()->setRange(0, 10000);
  icicle_image_.clear();

  drawIcicleTree();

  icicle_image_.update();
  repaint(); /// TODO(maxim): do I need this?
}

void
IcicleTreeCanvas::drawIcicleTree() {

  auto& na = *tc_.get_na();

  auto& root = *na[0];

  x_global_ = 0;
  cur_depth_ = 0;

  /// TODO(maxim): construct once, redraw many times
  processNode(root);

  // IcicleCursor icicle_cursor(root, tc_, na, icicle_image_);
  // PostorderNodeVisitor<IcicleCursor> visitor(icicle_cursor);

  // visitor.run();

}


std::pair<int, int>
IcicleTreeCanvas::processNode(const SpaceNode& node) {
  // auto yoff = _sa->verticalScrollBar()->value();

  ++cur_depth_;
  /// TODO(maxim): the fake (first) node returns 0 children (but has 1)
  /// in both Chuffed and Gecode
  const int kids = node.getNumberOfChildren();
  qDebug() << "kids: " << kids;
  auto& na = *tc_.get_na();

  int x_begin = 1000000;
  int x_end = 0;

  for (int i = 0; i < kids; ++i) {
    auto& kid = *node.getChild(na, i);

    auto extent = processNode(kid);
    auto x1 = extent.first;
    auto x2 = extent.second;
    qDebug() << "x1: " << x1 << "x2: " << x2;
    if (x1 < x_begin) x_begin = x1;
    if (x2 > x_end) x_end = x2;

  }

  if (kids == 0) {
    x_begin = x_global_;
    x_end = x_begin + 1;
    ++x_global_;
  }

  qDebug() << "x_begin: " << x_begin << " x_end: " << x_end;
  qDebug() << "cur_depth: " << cur_depth_;

  QRgb rect_color;
  switch (node.getStatus()) {
    case BRANCH: {
      rect_color = qRgb(50, 50, 255);
      break;
    }
    case FAILED: {
      rect_color = qRgb(255, 50, 50);
      break;
    }
    case SOLVED: {
      rect_color = qRgb(50, 255, 50);
      break;
    }
    default: {
      rect_color = qRgb(255, 255, 255);
    }
  }

  auto xoff = sa_.horizontalScrollBar()->value();
  icicle_image_.drawRect(x_begin - xoff, x_end - x_begin, cur_depth_, rect_color);

  --cur_depth_;

  return std::make_pair(x_begin, x_end);
}

void
IcicleTreeCanvas::sliderChanged(int) {
  /// calls redrawAll not more often than 60hz
  maybeCaller.call([this]() { redrawAll(); });
}




// /// **************** Cursor *********************

// IcicleCursor::IcicleCursor(VisualNode* root, TreeCanvas& tc,
//   const VisualNode::NodeAllocator& na, PixelImage& image)
//   : NodeCursor<VisualNode>(root, na), tc_(tc), na_(na), icicle_image_(image)
// {
//   max_depth_ = tc_.get_stats().maxDepth;
//   cur_depth_ = max_depth_;
// }

// void
// IcicleCursor::moveDownwards() {
//     ++cur_depth_;
//     NodeCursor<VisualNode>::moveDownwards();
// }


// void
// IcicleCursor::moveUpwards() {
//     --cur_depth_;
//     x = x1_;
//     NodeCursor<VisualNode>::moveUpwards();
// }

// void
// IcicleCursor::processCurrentNode() {

//   auto n = node();
//   x_++;

//   qDebug() << "x1: " << x1_ << "x2: " << x_;

//   icicle_image_.drawPixel(x_, cur_depth_, qRgb(0, 0, 0));

// }

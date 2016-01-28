
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

#include "icicle_tree_dialog.hh"
#include "treecanvas.hh"

using namespace cpprofiler::pixeltree;

IcicleTreeDialog::IcicleTreeDialog(TreeCanvas* tc): QDialog(tc) {

  this->resize(INIT_WIDTH, INIT_HEIGHT);
  this->setWindowTitle(QString::fromStdString(tc->getExecution()->getData()->getTitle()));

  scrollArea_ = new QAbstractScrollArea();

  auto layout = new QVBoxLayout();
  setLayout(layout);
  layout->addWidget(scrollArea_);

  canvas_ = new IcicleTreeCanvas(scrollArea_);

  connect(this, SIGNAL(windowResized()), canvas_, SLOT(resizeCanvas()));

}

void
IcicleTreeDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  emit windowResized();
}

IcicleTreeCanvas::IcicleTreeCanvas(QAbstractScrollArea* parent)
: QWidget(parent), sa_(*parent) {
  icicle_image_.drawPixel(10, 10, qRgb(0, 255, 0));
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
  icicle_image_.clear();

  icicle_image_.update();
  repaint(); /// TODO(maxim): do I need this?
}
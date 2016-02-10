
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

#ifndef CPPROFILER_PIXELVIEW_ICICLETREEDIALOG_HH
#define CPPROFILER_PIXELVIEW_ICICLETREEDIALOG_HH

#include <QDialog>
#include "pixelImage.hh"
#include "maybeCaller.hh"

class TreeCanvas;
class SpaceNode;
class QAbstractScrollArea;

 namespace cpprofiler { namespace pixeltree {

  class IcicleTreeCanvas;
  class IcicleTreeDialog;
  class IcicleCursor;

  class IcicleTreeDialog : public QDialog {
    Q_OBJECT

  private:
    /// TODO(maxim): make sure this gets deleted
    QAbstractScrollArea* scrollArea_;
    IcicleTreeCanvas* canvas_;

  Q_SIGNALS:
    void windowResized(void);

  public:
    explicit IcicleTreeDialog(TreeCanvas* tc);

    static const int INIT_WIDTH = 600;
    static const int INIT_HEIGHT = 400;

  protected:
    void resizeEvent(QResizeEvent * re);

  };

  class IcicleTreeCanvas : public QWidget {
    Q_OBJECT
  private:
    QAbstractScrollArea&  sa_;
    TreeCanvas& tc_;
    PixelImage icicle_image_;

    MaybeCaller maybeCaller;

    /// TODO(maxim): make this only accessable from within processNode
    int cur_depth_;
    int x_global_;

    /// TODO(maxim): temporarily here
    int icicle_width;

    void redrawAll();
    void drawIcicleTree();
    std::pair<int, int> processNode(const SpaceNode&);

  protected:
    void paintEvent(QPaintEvent* event);

  public:
    IcicleTreeCanvas(QAbstractScrollArea* parent, TreeCanvas* tc);

  public Q_SLOTS:
    void resizeCanvas(void);
    void sliderChanged(int value);
  };

}}


#endif
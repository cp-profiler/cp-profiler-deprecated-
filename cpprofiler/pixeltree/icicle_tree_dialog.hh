
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
class QComboBox;

namespace cpprofiler {
namespace pixeltree {

class IcicleTreeCanvas;
class IcicleTreeDialog;
class IcicleCursor;

struct IcicleRect {
  int x;
  int y;
  int width;
  int height;
  SpaceNode& node;
  IcicleRect(int x, int y, int width, int height, SpaceNode& node)
      : x(x), y(y), width(width), height(height), node(node) {}
};

struct IcicleNodeStatistic {
  int leafCnt, height;
};

class IcicleTreeDialog : public QDialog {
  Q_OBJECT

 private:
  /// TODO(maxim): make sure this gets deleted
  QAbstractScrollArea* scrollArea_;
  IcicleTreeCanvas* canvas_;

  Q_SIGNALS : void windowResized(void);

 public:
  explicit IcicleTreeDialog(TreeCanvas* tc);

  static const int INIT_WIDTH = 600;
  static const int INIT_HEIGHT = 400;

 protected:
  void resizeEvent(QResizeEvent* re);
};

enum class ColorMappingType { DEFAULT, DOMAIN_REDUCTION, NODE_TIME };

class IcicleTreeCanvas : public QWidget {
  Q_OBJECT
 private:
  QAbstractScrollArea& sa_;
  TreeCanvas& tc_;
  PixelImage icicle_image_;
  std::vector<IcicleRect> icicle_rects_;
  std::vector<SpaceNode*> nodes_selected;  // to know which nodes to deselect

  ColorMappingType color_mapping_type = ColorMappingType::DEFAULT;

  MaybeCaller maybeCaller;

  std::vector<IcicleNodeStatistic> statistic;

  int compressLevel;
  /// TODO(maxim): temporarily here
  float domain_red_sum;

  // init size of subtree
  IcicleNodeStatistic initTreeStatistic(SpaceNode& root, int idx);

  void redrawAll();
  void drawIcicleTree();
  void drawRects();
  void compressInit(SpaceNode& root, int idx);
  void dfsVisible(SpaceNode& root, int idx, int curx, int cury, int xoff, int width, int yoff, int depth);
  QRgb getColorByType(const SpaceNode& node);
  SpaceNode* getNodeByXY(int x, int y) const;

 protected:
  void paintEvent(QPaintEvent* event);
  void mousePressEvent(QMouseEvent* me);
  void mouseMoveEvent(QMouseEvent* me);

 public:
  IcicleTreeCanvas(QAbstractScrollArea* parent, TreeCanvas* tc);
  int getTreeHeight() { return statistic[0].height; }

 public Q_SLOTS:
  void resizePixel(int value);
  void resizeCanvas(void);
  void sliderChanged(int value);
  void changeColorMapping(const QString& text);
  void compressLevelChanged(int offset);
};
}
}

#endif

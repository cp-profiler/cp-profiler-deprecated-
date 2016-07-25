
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

#ifndef CPPROFILER_PIXELVIEW_PIXELTREEDIALOG_HH
#define CPPROFILER_PIXELVIEW_PIXELTREEDIALOG_HH

#include <QDialog>
#include <QLabel>
#include <QDebug>
#include <QAbstractScrollArea>

class QCheckBox;
class QAbstractScrollArea;
class TreeCanvas;

namespace cpprofiler {
namespace pixeltree {

class PixelTreeCanvas;

class InfoPanel {
  QLabel m_infoLabel;

  std::string m_varInfo;

public:
  QLabel* info_label() {
    return &m_infoLabel;
  }
  void set_var_info(const std::string& str) {
    m_varInfo = str;
    m_infoLabel.setText(str.c_str());
  }
};

class PixelTreeDialog : public QDialog {
  Q_OBJECT

 private:
  QAbstractScrollArea scrollArea;

  QCheckBox* time_cb;
  QCheckBox* domains_cb;
  QCheckBox* decision_vars_cb;
  QCheckBox* depth_analysis_cb;

  InfoPanel m_infoPanel;


  PixelTreeCanvas* canvas_;

  Q_SIGNALS :

  void windowResized(void);
  void signalPixelSelected(int);

 public:
  static const int INIT_WIDTH = 600;
  static const int INIT_HEIGHT = 400;

  explicit PixelTreeDialog(TreeCanvas* tc);

 protected:
  void resizeEvent(QResizeEvent* re);

 public Q_SLOTS:

  void setPixelSelected(int);
};
}
}

#endif
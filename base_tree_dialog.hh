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

#ifndef BASE_TREE_DIALOG_HH
#define BASE_TREE_DIALOG_HH

#include <QDialog>
#include <QLayout>

#include "execution.hh"

class Gist;
class QAbstractScrollArea;
class QStatusBar;
class QMenuBar;
class QMenu;
class QLabel;
class TreeCanvas;
enum class CanvasType;
class VisualNode;
class Statistics;

/// Abstract class
class BaseTreeDialog : public QDialog {
Q_OBJECT

  private:

    QHBoxLayout* main_layout;
    QVBoxLayout* status_layout;
    QGridLayout* layout;
    QVBoxLayout* nc_layout;
    QAbstractScrollArea* scrollArea;

    /// Status Bar
    QStatusBar* statusBar;

    void buildMenu(void);
    void connectSignals(void);

  protected:

    Gist* ptr_gist;

    /// Interface stuff
    QMenuBar* menuBar;
    QMenu* nodeMenu;

    TreeCanvas* _tc;
    QLabel* mergedLabel;
    QHBoxLayout* hbl;

  protected: /// Methods

    /// manage status message and show total time if finished (TODO: total time for merged tree?)
    void statusChangedShared(bool finished);

  public:

    BaseTreeDialog(Execution* execution, const CanvasType type, Gist* gist);
    ~BaseTreeDialog();

    /// **** GETTERS ****

    void setTitle(const std::string& file_name);
    TreeCanvas* getCanvas(void) { return _tc; }

    private Q_SLOTS:
    /// The status has changed (e.g., new solutions have been found)
    virtual void statusChanged(VisualNode*, const Statistics& stats, bool finished) = 0;
};

#endif

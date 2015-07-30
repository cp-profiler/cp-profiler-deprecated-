/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BASE_TREE_DIALOG_HH
#define BASE_TREE_DIALOG_HH

#include <QDialog>
#include <QLayout>

class Gist;
class ReceiverThread;
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

    BaseTreeDialog(ReceiverThread* receiver, const CanvasType type, Gist* gist);
    ~BaseTreeDialog();

    /// **** GETTERS ****

    void setTitle(const std::string& file_name);
    TreeCanvas* getCanvas(void) { return _tc; }

    private Q_SLOTS:
    /// The status has changed (e.g., new solutions have been found)
    virtual void statusChanged(VisualNode*, const Statistics& stats, bool finished) = 0;
};

#endif
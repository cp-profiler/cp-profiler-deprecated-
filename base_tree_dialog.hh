#ifndef BASE_TREE_DIALOG_HH
#define BASE_TREE_DIALOG_HH

#include "treecanvas.hh"

class Gist;
class ReceiverThread;

/// Abstract class
class BaseTreeDialog : public QDialog {
Q_OBJECT

  private:

    QHBoxLayout* main_layout;
    QVBoxLayout* status_layout;
    QGridLayout* layout;
    QVBoxLayout* nc_layout;
    QAbstractScrollArea* scrollArea;

    /// A menu bar
    QMenuBar* menuBar;
    

    /// Status Bar
    QStatusBar* statusBar;

    void buildMenu(void);
    void connectSignals(void);

  protected:

    Gist* ptr_gist;

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

    void setTitle(const char* file_name);
    TreeCanvas* getCanvas(void) { return _tc; }

    private Q_SLOTS:
    /// The status has changed (e.g., new solutions have been found)
    virtual void statusChanged(VisualNode*, const Statistics& stats, bool finished) = 0;
};

#endif
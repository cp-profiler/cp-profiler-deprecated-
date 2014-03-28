#ifndef PREFERENCES_HH
#define PREFERENCES_HH

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

/**
   * \brief Preferences dialog for %Gist
   */
class PreferencesDialog : public QDialog {
    Q_OBJECT

protected:
    QCheckBox* hideCheck;
    QCheckBox* zoomCheck;
    QCheckBox* smoothCheck;
    QSpinBox*  refreshBox;
    QCheckBox* slowBox;
    QCheckBox* moveDuringSearchBox;
protected Q_SLOTS:
    /// Write settings
    void writeBack(void);
    /// Reset to defaults
    void defaults(void);
    /// Toggle slow down setting
    void toggleSlow(int state);
public:
    /// Constructor
    PreferencesDialog(QWidget* parent = 0);

    /// Whether to automatically hide failed subtrees during search
    bool hideFailed;
    /// Whether to automatically zoom during search
    bool zoom;
    /// How often to refresh the display during search
    int refresh;
    /// Milliseconds to wait after each refresh (to slow down search)
    int refreshPause;
    /// Whether to use smooth scrolling and zooming
    bool smoothScrollAndZoom;
    /// Whether to move cursor during search
    bool moveDuringSearch;

};

#endif // PREFERENCES_HH

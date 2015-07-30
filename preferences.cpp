/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "preferences.hh"

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent) {
    QSettings settings("gecode.org", "Gist");
    hideFailed = settings.value("search/hideFailed", true).toBool();
    zoom = settings.value("search/zoom", false).toBool();
    refresh = settings.value("search/refresh", 500).toInt();
    refreshPause = settings.value("search/refreshPause", 0).toInt();
    smoothScrollAndZoom =
            settings.value("smoothScrollAndZoom", true).toBool();
    moveDuringSearch = false;

    hideCheck =
            new QCheckBox(tr("Hide failed subtrees automatically"));
    hideCheck->setChecked(hideFailed);
    zoomCheck =
            new QCheckBox(tr("Automatic zoom enabled on start-up"));
    zoomCheck->setChecked(zoom);
    smoothCheck =
            new QCheckBox(tr("Smooth scrolling and zooming"));
    smoothCheck->setChecked(smoothScrollAndZoom);

    QPushButton* defButton = new QPushButton(tr("Defaults"));
    QPushButton* cancelButton = new QPushButton(tr("Cancel"));
    QPushButton* okButton = new QPushButton(tr("Ok"));
    okButton->setDefault(true);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(defButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);

    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(defButton, SIGNAL(clicked()), this, SLOT(defaults()));
    connect(okButton, SIGNAL(clicked()), this, SLOT(writeBack()));

    QLabel* refreshLabel = new QLabel(tr("Display refresh rate:"));
    refreshBox  = new QSpinBox();
    refreshBox->setRange(0, 1000000);
    refreshBox->setValue(refresh);
    refreshBox->setSingleStep(100);
    QHBoxLayout* refreshLayout = new QHBoxLayout();
    refreshLayout->addWidget(refreshLabel);
    refreshLayout->addWidget(refreshBox);

    slowBox =
            new QCheckBox(tr("Slow down search"));
    slowBox->setChecked(refreshPause > 0);

    refreshBox->setEnabled(refreshPause == 0);

    connect(slowBox, SIGNAL(stateChanged(int)), this,
            SLOT(toggleSlow(int)));

    moveDuringSearchBox =
            new QCheckBox(tr("Move cursor during search"));
    moveDuringSearchBox->setChecked(moveDuringSearch);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(hideCheck);
    layout->addWidget(zoomCheck);
    layout->addWidget(smoothCheck);
    layout->addLayout(refreshLayout);
    layout->addWidget(slowBox);
    layout->addWidget(moveDuringSearchBox);

    QTabWidget* tabs = new QTabWidget;
    QWidget* page1 = new QWidget;
    page1->setLayout(layout);
    tabs->addTab(page1, "Drawing");

//    QLabel* cdlabel = new QLabel(tr("Commit distance:"));
    layout = new QVBoxLayout();
    QWidget* page2 = new QWidget;
    page2->setLayout(layout);
    tabs->addTab(page2, "Search");

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(tabs);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Preferences"));
}

void
PreferencesDialog::writeBack(void) {
    hideFailed = hideCheck->isChecked();
    zoom = zoomCheck->isChecked();
    refresh = refreshBox->value();
    refreshPause = slowBox->isChecked() ? 200 : 0;
    moveDuringSearch = moveDuringSearchBox->isChecked();
    smoothScrollAndZoom = smoothCheck->isChecked();
    QSettings settings("gecode.org", "Gist");
    settings.setValue("search/hideFailed", hideFailed);
    settings.setValue("search/zoom", zoom);
    settings.setValue("search/refresh", refresh);
    settings.setValue("search/refreshPause", refreshPause);
    settings.setValue("smoothScrollAndZoom", smoothScrollAndZoom);

    accept();
}

void
PreferencesDialog::defaults(void) {
    hideFailed = true;
    zoom = false;
    refresh = 500;
    refreshPause = 0;
    smoothScrollAndZoom = true;
    moveDuringSearch = false;
    hideCheck->setChecked(hideFailed);
    zoomCheck->setChecked(zoom);
    refreshBox->setValue(refresh);
    slowBox->setChecked(refreshPause > 0);
    smoothCheck->setChecked(smoothScrollAndZoom);
    moveDuringSearchBox->setChecked(moveDuringSearch);
}

void
PreferencesDialog::toggleSlow(int state) {
    refreshBox->setEnabled(state != Qt::Checked);
}

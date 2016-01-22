#include "profiler-conductor.hh"
#include "receiverthread.hh"
#include "profiler-tcp-server.hh"
#include "gistmainwindow.h"
#include "cmp_tree_dialog.hh"

#include <QPushButton>
#include <QVBoxLayout>

ProfilerConductor::ProfilerConductor()
    : QMainWindow()
{
    
    // this->setMinimumHeight(320);
    // this->setMinimumWidth(320);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    executionList = new QListWidget;
    executionList->setSelectionMode(QAbstractItemView::MultiSelection);

    QPushButton* gistButton = new QPushButton("show tree");
    connect(gistButton, SIGNAL(clicked(bool)), this, SLOT(gistButtonClicked(bool)));

    QPushButton* compareButton = new QPushButton("compare trees");
    connect(compareButton, SIGNAL(clicked(bool)), this, SLOT(compareButtonClicked(bool)));

    QPushButton* gatherStatisticsButton = new QPushButton("gather statistics");
    connect(gatherStatisticsButton, SIGNAL(clicked(bool)), this, SLOT(gatherStatisticsClicked(bool)));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(executionList);
    layout->addWidget(gistButton);
    layout->addWidget(compareButton);
    layout->addWidget(gatherStatisticsButton);
    centralWidget->setLayout(layout);

    // Listen for new executions.
    ProfilerTcpServer* listener = new ProfilerTcpServer(this);
    listener->listen(QHostAddress::Any, 6565);
}

class ExecutionListItem : public QListWidgetItem {
public:
    ExecutionListItem(Execution* execution, QListWidget* parent, int type = Type)
        : QListWidgetItem(parent, type),
          execution_(execution),
          gistWindow_(nullptr)
    {}

    Execution* execution_;
    GistMainWindow* gistWindow_;
};

void
ProfilerConductor::newExecution(Execution* execution) {
    qDebug() << "(maxim): new execution";
    ExecutionListItem *newItem = new ExecutionListItem(execution, executionList);
    newItem->setText("some execution");
    newItem->setSelected(true);
    executionList->addItem(newItem);
    executions << execution;

    connect(execution, SIGNAL(titleKnown()), this, SLOT(updateList()));
}

void
ProfilerConductor::updateList(void) {
    for (int i = 0 ; i < executions.size() ; i++) {
        executionList->item(i)->setText(QString::fromStdString(executions[i]->getDescription()));
    }
}

void
ProfilerConductor::gistButtonClicked(bool checked) {
    (void)checked;
    
    QList <QListWidgetItem*> selected = executionList->selectedItems();
    for (int i = 0 ; i < selected.size() ; i++) {
        ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);
        GistMainWindow* g = item->gistWindow_;
        if (g == nullptr) {
            g = new GistMainWindow(item->execution_, this);
            item->gistWindow_ = g;
        }
        g->show();
        g->activateWindow();
    }
}

void
ProfilerConductor::compareButtonClicked(bool checked) {
    (void)checked;

    QList <QListWidgetItem*> selected = executionList->selectedItems();
    if (selected.size() != 2) return;
    ExecutionListItem* item1 = static_cast<ExecutionListItem*>(selected[0]);
    ExecutionListItem* item2 = static_cast<ExecutionListItem*>(selected[1]);
    Execution* e = new Execution;

    if (item1->gistWindow_ == nullptr && item2->gistWindow_ == nullptr) return;

    CmpTreeDialog* ctd = new CmpTreeDialog(this, e, CanvasType::MERGED,
                                           item1->gistWindow_->getGist()->getCanvas(),
                                           item2->gistWindow_->getGist()->getCanvas());
    (void) ctd;
}

class StatsHelper {
    QString filename;
    StatsHelper(QString filename_) : filename(filename_) {}
    
};

void ProfilerConductor::gatherStatisticsClicked(bool checked) {
    (void) checked;

    QList <QListWidgetItem*> selected = executionList->selectedItems();
    for (int i = 0 ; i < selected.size() ; i++) {
        ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);
        GistMainWindow* g = item->gistWindow_;
        if (g == NULL) {
            g = new GistMainWindow(item->execution_, this);
            item->gistWindow_ = g;
        }
        //        g->hide();


        // This is a big mess.  The user must wait for the tree to be
        // built before they select their file.
        
        QString filename = QFileDialog::getSaveFileName(this, "Save statistics");
        // Very inelegant
        g->setStatsFilename(filename);

        g->gatherStatistics();
        // connect(g, SIGNAL(buildingFinished(void)),
        //         g, SLOT(gatherStatistics(void)));
        // g->show();
        // g->activateWindow();
    }
}

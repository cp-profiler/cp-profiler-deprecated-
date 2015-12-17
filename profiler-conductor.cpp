#include "profiler-conductor.hh"
#include "receiverthread.hh"
#include "profiler-tcp-server.hh"
#include "gistmainwindow.h"

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

    QPushButton* gistButton = new QPushButton("show tree");
    connect(gistButton, SIGNAL(clicked(bool)), this, SLOT(gistButtonClicked(bool)));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(executionList);
    layout->addWidget(gistButton);
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
          gistWindow_(NULL)
    {}

    Execution* execution_;
    GistMainWindow* gistWindow_;
};

void
ProfilerConductor::newExecution(Execution* execution) {
    ExecutionListItem *newItem = new ExecutionListItem(execution, executionList);
    newItem->setText("some execution");
    executionList->addItem(newItem);
    executions << execution;

    connect(execution, SIGNAL(newNode()), this, SLOT(updateList()));
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
        if (g == NULL) {
            g = new GistMainWindow(item->execution_, this);
            item->gistWindow_ = g;
        }
        g->show();
        g->activateWindow();
    }
}

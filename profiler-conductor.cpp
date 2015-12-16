#include "profiler-conductor.hh"
#include "receiverthread.hh"
#include "profiler-tcp-server.hh"

#include <QVBoxLayout>

ProfilerConductor::ProfilerConductor() {
    this->setMinimumHeight(320);
    this->setMinimumWidth(320);
    // QVBoxLayout *layout = new QVBoxLayout;

    executionList = new QListWidget(this);
    
    // // The list widget contains all the received executions.
    // executionListModel = new ExecutionListModel;
    
    // QListView* executionList = new QListView(this);
    executionList->setMinimumHeight(300);
    executionList->setMinimumWidth(300);
    // executionList->setModel(executionListModel);
    // layout->addWidget(executionList);

    // this->setLayout(layout);

    // Listen for new executions.
    ProfilerTcpServer* listener = new ProfilerTcpServer(this);
    listener->listen(QHostAddress::Any, 6565);
}

void
ProfilerConductor::newExecution(Execution* execution) {
    // ExecutionListItem *newItem = new ExecutionListItem(execution, executionList);
    QListWidgetItem *newItem = new QListWidgetItem(executionList);
    newItem->setText("some execution");
    executionList->addItem(newItem);
    executions << execution;

    // executionListModel->addExecution(execution);
    
    connect(execution, SIGNAL(newNode()), this, SLOT(updateList()));
}

void
ProfilerConductor::updateList(void) {
    for (int i = 0 ; i < executions.size() ; i++) {
        executionList->item(i)->setText(QString::fromStdString(executions[i]->getDescription()));
    }
}

// void ProfilerConductor::newSolverConnection() {
//     // This new connection gets its own thread.
//     ReceiverThread* receiver = new ReceiverThread(this);

//     Gist *gist = new Gist(this, receiver);
//     gist->show();
    
// }


    

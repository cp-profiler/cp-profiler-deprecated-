#include "profiler-tcp-server.hh"
#include "receiverthread.hh"
#include "execution.hh"

#include <QObject>
#include "qtgist.hh"
#include "gistmainwindow.h"

ProfilerTcpServer::ProfilerTcpServer(ProfilerConductor* parent)
    : QTcpServer(parent),
      _parent(parent)
{}

void ProfilerTcpServer::incomingConnection(qintptr socketDescriptor) {
    Execution* execution = new Execution();
    ReceiverThread* receiver = new ReceiverThread(socketDescriptor, execution, this);

    _parent->newExecution(execution);
    
    // GistMainWindow* g = new GistMainWindow(execution, _parent);
    // g->show();

    connect(receiver, SIGNAL(finished()), receiver, SLOT(deleteLater()));
    connect(receiver, SIGNAL(doneReceiving()), execution, SLOT(doneReceiving()));
    connect(receiver, SIGNAL(doneReceiving()), execution, SLOT(setDoneReceiving()));
    receiver->start();
}

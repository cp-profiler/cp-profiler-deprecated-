#include "profiler-tcp-server.hh"
#include "receiverthread.hh"
#include "qtgist.hh"
// #include <QObject>

#include "execution.hh"

ProfilerTcpServer::ProfilerTcpServer(ProfilerConductor* parent)
    : QTcpServer(parent), _parent(parent) {}

void ProfilerTcpServer::incomingConnection(qintptr socketDescriptor) {
  Execution* execution = new Execution();
  ReceiverThread* receiver =
      new ReceiverThread(socketDescriptor, execution, this);

  _parent->newExecution(execution);

  connect(receiver, SIGNAL(finished()), receiver, SLOT(deleteLater()));
  // already have this in receiverThread
  // connect(receiver, SIGNAL(doneReceiving()), execution,
  // SLOT(doneReceiving()));
  connect(receiver, SIGNAL(doneReceiving()), execution,
          SLOT(setDoneReceiving()));
  receiver->start();
}

#include "profiler-tcp-server.hh"
#include "receiverthread.hh"
#include "profiler-conductor.hh"

#include "execution.hh"

ProfilerTcpServer::ProfilerTcpServer(ProfilerConductor* parent)
    : QTcpServer{parent}, _conductor{*parent} {}

void ProfilerTcpServer::incomingConnection(qintptr socketDescriptor) {
  Execution* execution = new Execution();
  ReceiverThread* receiver =
      new ReceiverThread(socketDescriptor, execution, this);

  _conductor.newExecution(execution);

  receiver->start();
}

#include "profiler-tcp-server.hh"

ProfilerTcpServer::ProfilerTcpServer(std::function<void(qintptr)> callback)
    : QTcpServer{}, onConnectionCallback{callback} {}

// ProfilerTcpServer::~ProfilerTcpServer() {};

void ProfilerTcpServer::incomingConnection(qintptr socketDescriptor) {
  onConnectionCallback(socketDescriptor);
}

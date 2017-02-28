#ifndef PROFILER_TCP_SERVER_HH
#define PROFILER_TCP_SERVER_HH

#include <QTcpServer>
#include <functional>

class ProfilerConductor;

class ProfilerTcpServer : public QTcpServer {
  Q_OBJECT

 public:
  ProfilerTcpServer(std::function<void(qintptr)> callback);

 protected:
  void incomingConnection(qintptr socketDescriptor) override;

 private:
  std::function<void(qintptr)> onConnectionCallback;
};

#endif

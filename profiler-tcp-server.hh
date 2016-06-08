#ifndef PROFILER_TCP_SERVER_HH
#define PROFILER_TCP_SERVER_HH

#include <QTcpServer>

class ProfilerConductor;

class ProfilerTcpServer : public QTcpServer {
  Q_OBJECT

 public:
  ProfilerTcpServer(ProfilerConductor* parent);

 protected:
  void incomingConnection(qintptr socketDescriptor) override;

 private:
  ProfilerConductor& _conductor;
};

#endif

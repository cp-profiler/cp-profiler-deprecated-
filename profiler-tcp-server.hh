#ifndef PROFILER_TCP_SERVER_HH
#define PROFILER_TCP_SERVER_HH

#include <QTcpServer>
#include "profiler-conductor.hh"

class ProfilerTcpServer : public QTcpServer {
  Q_OBJECT

 public:
  ProfilerTcpServer(ProfilerConductor* parent = 0);

 protected:
  void incomingConnection(qintptr socketDescriptor) Q_DECL_OVERRIDE;

 private:
  ProfilerConductor* _parent;
};

#endif

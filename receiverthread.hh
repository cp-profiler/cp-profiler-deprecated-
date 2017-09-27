/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef receiver_THREAD_HH
#define receiver_THREAD_HH

#include <QThread>
#include <QObject>
#include <QTcpSocket>

#include <QDebug>
#include <iostream>

#include "submodules/cpp-integration/message.hpp"

class Execution;

class ReceiverThread : public QThread {
  Q_OBJECT

 public:
  ReceiverThread(int socketDescriptor, Execution* execution,
                 QObject* parent = 0);


  int socketDescriptor;

  ~ReceiverThread() = default;

 signals:
  void doneReceiving(void);
  void executionIdReady(Execution*);
  void executionStarted(Execution*);

 private:
  void run(void) override;

  QByteArray* buffer;
  QTcpSocket* tcpSocket;
  int size;
  Execution* execution = nullptr;
};

class ReceiverWorker : public QObject {
  Q_OBJECT
 public:
  ReceiverWorker(QTcpSocket* socket, Execution* execution)
      : execution(execution), tcpSocket(socket) {
  }

 signals:
  void doneReceiving(void);
  void executionIdReady(Execution*);
  void executionStarted(Execution*);

 private:
  Execution* execution;
  QByteArray buffer;
  bool size_read = false;

  // size of the following message in bytes
  int msg_size = 0;
  /// messages processed since last buffer reset
  int msg_processed = 0;
  /// where to read next from
  int bytes_read = 0;

  cpprofiler::MessageMarshalling marshalling;

  /// if false -> next to read is size, otherwise -- message itself
  /// number of messages per buffer reset
  static constexpr int MSG_PER_BUFFER = 10000;

  QTcpSocket* tcpSocket;

  bool execution_id_communicated = false;
  /// initialised shortly after execution id is available
  bool wait_for_name_map = true;
  void handleStartMessage(const cpprofiler::Message& msg);
 public slots:
  void doRead();
  void handleMessage(const cpprofiler::Message& msg);
};

#endif

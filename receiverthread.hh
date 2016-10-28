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

#include "message.pb.hh"

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

 private:
  void run(void) override;

  QByteArray* buffer;
  QTcpSocket* tcpSocket;
  int size;
  Execution* execution;
};

class ReceiverWorker : public QObject {
  Q_OBJECT
 public:
  ReceiverWorker(QTcpSocket* socket, Execution* execution)
      : execution(execution), tcpSocket(socket) {
    size = 0;
  }

 signals:
  void doneReceiving(void);

 private:
  Execution* execution;
  QByteArray buffer;
  int size;
  QTcpSocket* tcpSocket;
 public slots:
  void doRead();
};

#endif
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

  // friend Gist;

public:

    ReceiverThread(int socketDescriptor, Execution* execution, QObject* parent = 0);
  // void switchCanvas(TreeCanvas* tc);
  // void receive(TreeCanvas* tc);


    //private:
  // TreeCanvas* _t;
  // Gist* ptr_gist;

    // Execution* execution;

//   volatile bool _quit;

  int socketDescriptor;

    ~ReceiverThread() {
        std::cerr << "Receiver thread " << this << " being destroyed\n";
    }

// public Q_SLOTS:
//   void updateCanvas(void); /// TODO: should be moved to TreeCanvas
//   void stopThread(void);

signals:
  // void update(int w, int h, int scale0);
  void startReceiving(void);
  void doneReceiving(void);
  // void statusChanged(bool);
  // void newCanvasNeeded(void);

  /// Emit when receives new nodes to update Status Bar
  // void receivedNodes(bool finished);

    // void newNode(message::Node& node);

// private slots:
//     void readyRead();

protected:
  void run(void);

    QByteArray* buffer;
    QTcpSocket* tcpSocket;
    int size;
    Execution* execution;
};




inline
quint32 ArrayToInt(const QByteArray& ba) {
    const char* p = ba.data();
    return *(reinterpret_cast<const quint32*>(p));
}


class ReceiverWorker : public QObject {
    Q_OBJECT
public:
    ReceiverWorker(QTcpSocket* socket, Execution* execution) : execution(execution), tcpSocket(socket) {
        size = 0;
    }

signals:
    void startReceiving(void);
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



/// ***************************************
/// some of the stuff I got rid of for now:

// private:
// int depth
// bool a

// Q_SIGNALS:
// void scaleChanged(int);     // seems like scale works even without int
// void solution(void);        // got rid of since don't have Space
// void searchFinished(void);  // dont have the function in TreeCanvas anymore
// void moveToNode(VisualNode* n, bool); //

/// **************************************

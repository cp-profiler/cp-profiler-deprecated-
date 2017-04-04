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

#include "receiverthread.hh"
#include "globalhelper.hh"
#include "message.pb.hh"

#include <QTcpSocket>
#include "execution.hh"
#include <third-party/json.hpp>

// This is a bit wrong.  We have both a separate thread and
// asynchronous reading from the socket.  One or the other would
// suffice.

ReceiverThread::ReceiverThread(int socketDescriptor, Execution* execution, QObject* parent)
    : QThread(parent),
      socketDescriptor(socketDescriptor),
      execution(execution)
{
    connect(this, SIGNAL(doneReceiving()), execution, SIGNAL(doneReceiving()));
}


void
ReceiverThread::run(void) {
    tcpSocket = new QTcpSocket;
    if (!tcpSocket->setSocketDescriptor(socketDescriptor)) {
        std::cerr << "something went wrong setting the socket descriptor\n";
        abort();
    }

    /// TODO(maxim): memory leak here
    ReceiverWorker* worker = new ReceiverWorker(tcpSocket, execution);
    connect(tcpSocket, SIGNAL(readyRead()), worker, SLOT(doRead()));

    connect(worker, SIGNAL(doneReceiving(void)), this, SIGNAL(doneReceiving(void)));
    connect(worker, SIGNAL(doneReceiving(void)), this, SLOT(quit(void)));

    connect(tcpSocket, SIGNAL(disconnected(void)), this, SLOT(quit(void)));
    connect(tcpSocket, SIGNAL(disconnected(void)), this, SIGNAL(doneReceiving(void)));

    // std::cerr << "Receiver thread " << this << " running event loop\n";
    exec();
    // std::cerr << "Receiver thread " << this << " terminating\n";
}

static quint32 ArrayToInt(const QByteArray& ba) {
  const char* p = ba.data();
  return *(reinterpret_cast<const quint32*>(p));
}


// This function is called whenever there is new data available to be
// read on the socket.
void
ReceiverWorker::doRead()
{
    QByteArray data;
    while (tcpSocket->bytesAvailable() > 0) {
        // Read all data on the socket into a local buffer.
        buffer.append(tcpSocket->readAll());

        // This counts how far into "buffer" we are.
        int bytes_read = 0;

        // While we have enough data in the buffer to process the next
        // header or body
        while ( (size == 0 && buffer.size() >= bytes_read + 4)
             || (size > 0  && buffer.size() >= bytes_read + size)) {
            // Read the header (which contains the size of the body)
            if (size == 0 && buffer.size() >= bytes_read + 4) {
                size = ArrayToInt(buffer.mid(bytes_read, 4));
                bytes_read += 4;
            }
            // Read the body
            if (size > 0 && buffer.size() >= bytes_read + size) {
                message::Node msg1;
                msg1.ParseFromArray(buffer.data() + bytes_read, size);
                bytes_read += size;

                size = 0;

                switch (msg1.type()) {
                case message::Node::NODE:
                    execution->handleNewNode(msg1);
                    break;
                case message::Node::START:
                {
                    if (msg1.has_info()) {
                        std::string s = msg1.info();
                        auto info_json = nlohmann::json::parse(msg1.info());

                        auto execution_id = info_json.find("execution_id");
                        if(execution_id != info_json.end()) {
                            execution->setExecutionId(*execution_id);
                        }

                        auto variableListString = info_json.find("variable_list");
                        if(variableListString != info_json.end()) {
                            execution->setVariableListString(*variableListString);
                        }
                    }

                    if (msg1.restart_id() != -1 && msg1.restart_id() != 0) {
                        // qDebug() << ">>> restart and continue";
                        break;
                    }

                    bool is_restarts = (msg1.restart_id() != -1);

                    execution->start(msg1.label(), is_restarts);
                }
                break;
                case message::Node::DONE:
                    emit doneReceiving();
                    break;
                }
            }
        }

        // Discard from the buffer the part we have read.
        buffer.remove(0, bytes_read);
    }
}

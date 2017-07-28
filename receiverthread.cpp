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

#include <QTcpSocket>
#include <QMutex>
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
    connect(worker, &ReceiverWorker::executionIdReady, [this](Execution* ex){

        /// TODO(maxim): should this really be run for every solver thread?
        connect(this, SIGNAL(doneReceiving()), ex, SIGNAL(doneReceiving()));
        emit executionIdReady(ex);
    });

    connect(tcpSocket, SIGNAL(disconnected(void)), this, SLOT(quit(void)));
    connect(tcpSocket, SIGNAL(disconnected(void)), this, SIGNAL(doneReceiving(void)));

    // std::cerr << "Receiver thread " << this << " running event loop\n";
    exec();
    // std::cerr << "Receiver thread " << this << " terminating\n";
}

static int32_t ArrayToInt(const QByteArray& ba) {

  const char* p = ba.data();
  return *(reinterpret_cast<const quint32*>(p));
}

using cpprofiler::Message;

void ReceiverWorker::handleMessage(const Message& msg) {

    switch (msg.type()) {
        case cpprofiler::MsgType::NODE:
            execution->handleNewNode(msg);
        break;
        case cpprofiler::MsgType::START:
            // std::cerr << "START\n";

            /// Not very first START (in case of restart execution)
            if (msg.restart_id() != -1 && msg.restart_id() != 0) {
                break;
            }

            /// ----- Very first START -----
            {
                // execution = new Execution();
                bool is_restarts = (msg.restart_id() != -1);
                execution->start(msg.label(), is_restarts);
            }

            if (msg.has_info()) {

                try {
                    std::string s = msg.info();

                    auto info_json = nlohmann::json::parse(msg.info());

                    auto execution_id = info_json.find("execution_id");
                    if(execution_id != info_json.end()) {

                        if (!execution_id_communicated) {
                            execution->setExecutionId(*execution_id);
                            emit executionIdReady(execution);
                        }
                    }

                    auto variableListString = info_json.find("variable_list");
                    if(variableListString != info_json.end()) {
                        execution->setVariableListString(*variableListString);
                    }
                } catch (std::exception& e) {
                    std::cerr << "Can't parse json in info: " << e.what() << "\n";
                }
            }

            // qDebug() << "waiting for cond var";

            /// NOTE(maxim): for now, don't want to wait
#ifdef MAXIM_DEBUG
            execution_id_communicated = true;
#endif

            /// Wait for condition variable
            if (!execution_id_communicated) {
                QMutex m;
                m.lock();
                while (!execution->has_exec_id) {
                    execution->has_exec_id_cond.wait(&m);
                }
                // qDebug() << "exec id communicated";
                execution_id_communicated = true;
                m.unlock();
            }

        break;
        case cpprofiler::MsgType::DONE:
            emit doneReceiving();
            std::cerr << "DONE\n";
        break;
        default:
            std::cerr << "unknown type\n";
    }

}

// This function is called whenever there is new data available to be
// read on the socket.
void
ReceiverWorker::doRead()
{
    bool can_read_more = true;

    while (tcpSocket->bytesAvailable() > 0 || can_read_more) {

        if (tcpSocket->bytesAvailable() > 0) can_read_more = true;

        buffer.append(tcpSocket->readAll());

        if (!size_read) {

            if (buffer.size() < bytes_read + 4) {
                can_read_more = false;
                continue;
            }

            msg_size = ArrayToInt(buffer.mid(bytes_read, 4));

            bytes_read += 4;
            size_read = true;
            // qDebug() << "size read";

        } else {

            if (buffer.size() < bytes_read + msg_size) {
                can_read_more = false;
                continue;
            }


            marshalling.deserialize(buffer.data() + bytes_read, msg_size);

            auto msg = marshalling.get_msg();

            handleMessage(msg);

            bytes_read += msg_size;
            size_read = false;
            msg_processed++;

            /// reset the buffer every MSG_PER_BUFFER messages
            if (msg_processed == MSG_PER_BUFFER) {
                msg_processed = 0;
                buffer.remove(0, bytes_read);
                bytes_read = 0;
            }

            // qDebug() << "message read";
        }
    }

}

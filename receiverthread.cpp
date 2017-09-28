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
    connect(tcpSocket, &QTcpSocket::readyRead, worker, &ReceiverWorker::doRead);

    auto conn_1 = std::make_shared<QMetaObject::Connection>();

    *conn_1 = connect(worker, &ReceiverWorker::doneReceiving, [this, conn_1]() {
        qDebug() << "worker::doneReceiving";
        QObject::disconnect(*conn_1);
        emit doneReceiving();
        quit();
    });

    connect(worker, &ReceiverWorker::executionIdReady, [this](Execution* ex) {
        emit executionIdReady(ex);
    });

    auto conn_2 = std::make_shared<QMetaObject::Connection>();

    *conn_2 = connect(worker, &ReceiverWorker::executionStarted, [this, conn_2](Execution* ex) {

        /// NOTE(maxim): this can be triggered more than once...
        connect(this, SIGNAL(doneReceiving()), ex, SIGNAL(doneReceiving()));
        QObject::disconnect(*conn_2);
        emit executionStarted(ex);
    });

    // connect(tcpSocket, &QTcpSocket::disconnected, this, &ReceiverThread::doneReceiving);
    connect(tcpSocket, &QTcpSocket::disconnected, [this]() {
        qDebug() << "tcpSocket->disconnected";
        this->doneReceiving();
    });


    // connect(tcpSocket, QTcpSocket::disconnected, this, ReceiverThread::quit);
    connect(tcpSocket, &QTcpSocket::disconnected, [this]() {
        this->quit();
    });

    exec();
}

static int32_t ArrayToInt(const QByteArray& ba) {

  const char* p = ba.data();
  return *(reinterpret_cast<const quint32*>(p));
}

using cpprofiler::Message;

void ReceiverWorker::handleStartMessage(const Message& msg) {

    qDebug() << "START";

    std::string execution_name = "<no name>";
    bool has_restarts = false;

    if (msg.has_info()) {

        try {
            auto info_json = nlohmann::json::parse(msg.info());
            auto has_restarts_it = info_json.find("has_restarts");
#ifdef MAXIM_DEBUG
            if(has_restarts_it != info_json.end()) {
                qDebug() << "has_restarts: " << has_restarts_it->get<bool>();
                has_restarts = has_restarts_it->get<bool>();
            }
#endif
            auto name = info_json.find("name");
            if (name != info_json.end()) {
                execution_name = *name;
                qDebug() << "execution name:" << execution_name.c_str();
            }

            auto exec_id = info_json.find("execution_id");
            if (exec_id != info_json.end()) {

                qDebug() << "execution id:" << exec_id->get<int>();

                if(exec_id != info_json.end()) {
                    if (!execution_id_communicated) {
                        execution->setExecutionId(exec_id->get<int>());
                        emit executionIdReady(execution);
                    }
                } else {
                    /// Note(maxim): should not expect Execution Id in the future,
                    /// so there is no reason to wait for the name map
                    wait_for_name_map = false;
                }
            }


        } catch (std::exception& e) {
            std::cerr << "Can't parse json in info: " << e.what() << "\n";
        }

    }

    execution->begin(msg.label(), has_restarts);
    emit executionStarted(execution);

    /// Wait for condition variable
    if (!execution_id_communicated && wait_for_name_map) {
        QMutex m;
        m.lock();
        while (!execution->has_exec_id) {
            execution->has_exec_id_cond.wait(&m);
        }
        execution_id_communicated = true;
        m.unlock();
    }
}

void ReceiverWorker::handleMessage(const Message& msg) {

    switch (msg.type()) {
        case cpprofiler::MsgType::NODE:
            execution->handleNewNode(msg);
        break;
        case cpprofiler::MsgType::START:
            handleStartMessage(msg);
        break;
        case cpprofiler::MsgType::DONE:
            emit doneReceiving();
            std::cerr << "DONE\n";
        break;
        case cpprofiler::MsgType::RESTART:
            std::cerr << "RESTART\n";
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

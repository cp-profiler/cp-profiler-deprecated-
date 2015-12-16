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
#include "solver_tree_dialog.hh"
#include "message.pb.hh"
// #include "third-party/nn.hpp"
// #include <nanomsg/pipeline.h>

#include <QTcpSocket>

// This is all a bit wrong.  We have both a separate thread and
// asynchronous reading from the socket.  One or the other would
// suffice.

//class SolverTreeDialog;

ReceiverThread::ReceiverThread(int socketDescriptor, Execution* execution, QObject* parent)
    : QThread(parent),
      socketDescriptor(socketDescriptor),
      execution(execution)
{
//    connect(this, SIGNAL(newNode(message::Node&)), execution, SLOT(handleNewNode(message::Node&)));
    connect(this, SIGNAL(startReceiving()), execution, SIGNAL(startReceiving()));
    connect(this, SIGNAL(doneReceiving()), execution, SIGNAL(doneReceiving()));

    // ptr_gist = static_cast<Gist*>(parent);
}


void
ReceiverThread::run(void) {

    // nn::socket socket (AF_SP, NN_PULL);

    // QString port  = GlobalParser::value(GlobalParser::port_option());
    // QString address = QString("tcp://*:") + port;

    // try {
    //     socket.bind(address.toStdString().c_str());
    //     qDebug() << "connected to" << port;
    // } catch (std::exception& e) {
    //     std::cerr << "error connecting to socket: "
    //               << address.toStdString().c_str() << std::endl;
    //     std::cerr << '"' << e.what() << '"' << std::endl;
    //     abort();
    // }

    tcpSocket = new QTcpSocket;
    if (!tcpSocket->setSocketDescriptor(socketDescriptor)) {
        std::cerr << "something went wrong setting the socket descriptor\n";
        abort();
    }

    ReceiverWorker* worker = new ReceiverWorker(tcpSocket, execution);
    connect(tcpSocket, SIGNAL(readyRead()), worker, SLOT(doRead()));

    connect(worker, SIGNAL(startReceiving(void)), this, SIGNAL(startReceiving(void)));
    connect(worker, SIGNAL(doneReceiving(void)), this, SIGNAL(doneReceiving(void)));

    // buffer = new QByteArray();
    // nodeCount = 0;
    // size = 0;
    
    // connect(tcpSocket, SIGNAL(readyRead()), SLOT(readyRead()));
    // connect(tcpSocket, SIGNAL(disconnected()),
    // SLOT(disconnected()));

    std::cerr << "Receiver thread " << this << " running event loop\n";
    exec();

    std::cerr << "Receiver thread " << this << " waiting for TCP connection to finish\n";
    tcpSocket->waitForDisconnected(-1);
    std::cerr << "Socket for receiver thread " << this << " has disconnected; terminating thread\n";
}


void
ReceiverWorker::doRead()
{
        // std::cerr << "doRead\n";
        while (tcpSocket->bytesAvailable() > 0) {
            // std::cerr << "doRead: " << tcpSocket->bytesAvailable() << " bytes available; reading...\n";
            QByteArray buf = tcpSocket->readAll();
            buffer.append(buf);
            while ((size == 0 && buffer.size() >= 4) || (size > 0 && buffer.size() >= size)) {
                // std::cerr << "target size: " << size << "; buffer size: " << buffer.size() << "\n";
                if (size == 0 && buffer.size() >= 4) {
                    size = ArrayToInt(buffer.mid(0,4));
                    // std::cout << "Read message size: " << size << "\n";
                    buffer.remove(0,4);
                }
                // std::cerr << "target size: " << size << "; buffer size: " << buffer.size() << "\n";
                if (size > 0 && buffer.size() >= size) {
                    QByteArray data = buffer.mid(0,size);
                    buffer.remove(0, size);
                    int msgsize = size;
                    size = 0;

                    message::Node msg1;
                    msg1.ParseFromArray(data, msgsize);

                    std::cerr << "message type: " << msg1.type() << "\n";

                    switch (msg1.type()) {
                    case message::Node::NODE:
                        // _t->_data->handleNodeCallback(msg1);
                        // emit newNode(&msg1);
                        execution->handleNewNode(msg1);
                        ++nodeCount;
                        break;
                    case message::Node::START: /// TODO: start sending should have model name
                        qDebug() << "START RECEIVING";
                    
                        // if (msg->restart_id == -1 || msg->restart_id == 0) { // why 1?
                    
                    
                        if (msg1.restart_id() != -1 && msg1.restart_id() != 0) {
                            qDebug() << ">>> restart and continue";
                            break;
                        }

                        /// restart_id either -1 or 0 below this point:

                        /// Check if new canvas needs to be created
                        // if (ptr_gist->sndCanvas->isChecked() && ptr_gist->canvas->_isUsed) {
                        //         emit newCanvasNeeded();

                        //     _t = ptr_gist->getLastCanvas();
                        //     ptr_gist->getLastTreeDialog()->setTitle(msg1.label());
                        //     ptr_gist->connectCanvas(_t);
                        //     qDebug() << "Switched to another canvas";
                        // } else {
                        //     /// set Title through ptr_gist
                        //     qDebug() << "here: " << _t->getData()->getTitle().c_str();
                        //     ptr_gist->emitChangeMainTitle(msg1.label());
                        // }

                        // if (msg1.restart_id() == -1) {
                        //     ptr_gist->canvasTwo = NULL; /// TODO: do I still need this?

                        //     _t->reset(false); // no restarts
                        //     emit startReceiving();
                        // }
                        // else if (msg1.restart_id() == 0){

                        //     _t->reset(true);
                        //     emit startReceiving();
                        //     qDebug() << ">>> new restart";
                        // }

                        emit startReceiving();

                        // _t->getData()->setTitle(msg1.label());
                        break;
                    case message::Node::DONE:
                        qDebug() << "received DONE SENDING";
                        // updateCanvas();
                        /// needed for Opturion CPX restarts
                        // if (!_t->_data->isRestarts())
                        emit doneReceiving();
                        // disconnect(this, SIGNAL(startReceiving(void)),
                        //            _t->_builder, SLOT(startBuilding(void)));
                        // disconnect(this, SIGNAL(receivedNodes(bool)), _t,
                        //         SLOT(statusChanged(bool)));


                        break;
                    }
                }
            }
        }

    }

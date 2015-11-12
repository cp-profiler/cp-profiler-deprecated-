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
#include "third-party/nn.hpp"
#include <nanomsg/pipeline.h>

class SolverTreeDialog;

ReceiverThread::ReceiverThread(QWidget* parent): QThread(parent) {
    ptr_gist = static_cast<Gist*>(parent);
}

void
ReceiverThread::receive(TreeCanvas* tc) {

  qDebug() << "in ReceiverThread::receive";
  _t = tc;
  _quit = false;

  start();
}

void
ReceiverThread::stopThread(void) {
    qDebug() << "stoping thread";
    _quit = true;
}


void
ReceiverThread::switchCanvas(TreeCanvas* tc) { // not needed actually
    _t = tc;
}

void
ReceiverThread::run(void) {

    nn::socket socket (AF_SP, NN_PULL);

    QString port  = GlobalParser::value(GlobalParser::port_option());
    QString address = QString("tcp://*:") + port;

    try {
        socket.bind(address.toStdString().c_str());
        qDebug() << "connected to" << port;
    } catch (std::exception& e) {
        std::cerr << "error connecting to socket: "
                  << address.toStdString().c_str() << std::endl;
        std::cerr << '"' << e.what() << '"' << std::endl;
        abort();
    }

    int nodeCount = 0;

    while (!_quit) {

        // Possible optimisation: allocate this buffer outside the
        // loop; currently nanomsg will allocate a new one every time.
        void *buf = NULL;
        int bytes = socket.recv(&buf, NN_MSG, NN_DONTWAIT); // non-blocking so I can exit any time

        if (bytes == -1 && errno == EAGAIN) {
            msleep(100);
            continue;
        }

        message::Node msg1;
        msg1.ParseFromArray(buf, bytes);

        switch (msg1.type()) {
            case message::Node::NODE:
                _t->_data->handleNodeCallback(msg1);
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
                if (ptr_gist->sndCanvas->isChecked() && ptr_gist->canvas->_isUsed) {
                        emit newCanvasNeeded();

                    _t = ptr_gist->getLastCanvas();
                    ptr_gist->getLastTreeDialog()->setTitle(msg1.label());
                    ptr_gist->connectCanvas(_t);
                    qDebug() << "Switched to another canvas";
                } else {
                    /// set Title through ptr_gist
                    qDebug() << "here: " << _t->getData()->getTitle().c_str();
                    ptr_gist->emitChangeMainTitle(msg1.label());
                }

                if (msg1.restart_id() == -1) {
                    ptr_gist->canvasTwo = NULL; /// TODO: do I still need this?

                    _t->reset(false); // no restarts
                    emit startReceiving();
                }
                else if (msg1.restart_id() == 0){

                    _t->reset(true);
                    emit startReceiving();
                    qDebug() << ">>> new restart";
                }

                _t->getData()->setTitle(msg1.label());
            break;
            case message::Node::DONE:
                qDebug() << "received DONE SENDING";
                updateCanvas();
                /// needed for Opturion CPX restarts
                // if (!_t->_data->isRestarts())
                    emit doneReceiving();
                    disconnect(this, SIGNAL(startReceiving(void)),
                               _t->_builder, SLOT(startBuilding(void)));
                    disconnect(this, SIGNAL(receivedNodes(bool)), _t,
                            SLOT(statusChanged(bool)));


            break;
        }

        /// TODO: this part should be moved into treeCanvas
        if (_t->refresh > 0 && nodeCount >= _t->refresh) {
            _t->currentNode->dirtyUp(*_t->na);
            updateCanvas();

            // emit statusChanged(false);
            emit receivedNodes(false);
            nodeCount = 0;
            if (_t->refreshPause > 0)
              msleep(_t->refreshPause);
        }

    }

    qDebug() << "quiting run";

}

void
ReceiverThread::updateCanvas(void) {

    // qDebug() << "update Canvas" << _t->_id;

  _t->layoutMutex.lock();

    if (_t->root == NULL) return;

    if (_t->autoHideFailed) {
        _t->root->hideFailed(*_t->na, true);
    }

    for (VisualNode* n = _t->currentNode; n != NULL; n=n->getParent(*_t->na)) {
        if (n->isHidden()) {
            _t->currentNode->setMarked(false);
            _t->currentNode = n;
            _t->currentNode->setMarked(true);
            break;
        }
    }


    _t->root->layout(*_t->na);
    BoundingBox bb = _t->root->getBoundingBox();

    int w = static_cast<int>((bb.right-bb.left+Layout::extent)*_t->scale);
    int h = static_cast<int>(2*Layout::extent+
                             _t->root->getShape()->depth()
                             *Layout::dist_y*_t->scale);
    _t->xtrans = -bb.left+(Layout::extent / 2);

    int scale0 = static_cast<int>(_t->scale*100);
    if (_t->autoZoom) {
        QWidget* p = _t->parentWidget();
        if (p) {
            double newXScale =
                    static_cast<double>(p->width()) / (bb.right - bb.left +
                                                       Layout::extent);
            double newYScale =
                    static_cast<double>(p->height()) /
                    (_t->root->getShape()->depth() * Layout::dist_y + 2*Layout::extent);

            scale0 = static_cast<int>(std::min(newXScale, newYScale)*100);
            if (scale0<LayoutConfig::minScale)
                scale0 = LayoutConfig::minScale;
            if (scale0>LayoutConfig::maxAutoZoomScale)
                scale0 = LayoutConfig::maxAutoZoomScale;
            double scale = (static_cast<double>(scale0)) / 100.0;

            w = static_cast<int>((bb.right-bb.left+Layout::extent)*scale);
            h = static_cast<int>(2*Layout::extent+
                                 _t->root->getShape()->depth()*Layout::dist_y*scale);
        }
    }

    _t->layoutMutex.unlock();
    emit update(w,h,scale0);
}

#include "recieverthread.hh"

RecieverThread::RecieverThread(QWidget* parent): QThread(parent) {
    ptr_gist = static_cast<Gist*>(parent);
}

void
// RecieverThread::recieve(VisualNode* n, TreeCanvas* tc) {
RecieverThread::recieve(TreeCanvas* tc) {
//  QMutexLocker locker(&mutex);
  /// skipped smth here
 // _node = n;
  
  _t = tc;
  
  start();
}

void
RecieverThread::switchCanvas(TreeCanvas* tc) { // not needed actually
    _t = tc;
}

void
RecieverThread::run(void) {

    zmq::context_t context(1);
    zmq::socket_t socket (context, ZMQ_PULL);
    try {
        socket.bind("tcp://*:6565");
        // qDebug() << "connected to 6565";
    } catch (std::exception& e) {
        std::cerr << "error connecting to socket\n";
    }
    
    int nodeCount = 0;
    while (true) {
        zmq::message_t request;

        socket.recv (&request);

        Message *msg = reinterpret_cast<Message*>(request.data());

        switch (msg->type) {
            case NODE_DATA:
                _t->_data->handleNodeCallback(msg);
                ++nodeCount;
            break;
            case START_SENDING:
                /// start building the tree

                // if (msg->restart_id == -1 || msg->restart_id == 1) { // why 1?
                if (msg->restart_id == -1) {
                    ptr_gist->canvasTwo = NULL;
                    if (ptr_gist->sndCanvas->isChecked() && ptr_gist->canvas->_isUsed) {

                        emit newCanvasNeeded(); // shows the 2nd canvas
                        while (!ptr_gist->canvasTwo) {
                            // qDebug() << "no canvas yet";
                        }

                        _t = ptr_gist->canvasTwo;
                        ptr_gist->connectCanvas(_t, ptr_gist->canvas);
                        qDebug() << "Switched to another canvas";
                    }

                    _t->reset(false); // no restarts
                    emit startWork();
                } else if (msg->restart_id == 0){

                    _t->reset(true);
                    emit startWork();
                    qDebug() << ">>> new restart";
                } else {
                    qDebug() << ">>> restart and continue";
                }
            break;
            case DONE_SENDING:
                qDebug() << "Done receiving";
                updateCanvas();
                if (!_t->_data->isRestarts())
                    _t->_data->setDone();

            break;
        }

        if (_t->refresh > 0 && nodeCount >= _t->refresh) {
            _t->currentNode->dirtyUp(*_t->na);
            updateCanvas();
            emit statusChanged(false);
            nodeCount = 0;
            if (_t->refreshPause > 0)
              msleep(_t->refreshPause);
        }

    }

}

void
RecieverThread::updateCanvas(void) {

  // if (t == NULL) return; /// TODO: why do I need this all of a sudden?
  _t->layoutMutex.lock();

    if (_t->root == NULL) return;

    /// does this ever happen?
    if (_t->autoHideFailed) {
        _t->root->hideFailed(*_t->na,true);
    }

    /// mark as dirty?
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

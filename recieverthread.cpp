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
  
  t = tc;
  
  start();
  qDebug() << "in the bottom of RecieverThread::recieve";

}

void
RecieverThread::switchCanvas(TreeCanvas* tc) { // not needed actually
    t = tc;
}

void
RecieverThread::run(void) {

    zmq::context_t context(1);
    zmq::socket_t socket (context, ZMQ_PULL);
    try {
        socket.bind("tcp://*:6565");
        qDebug() << "connected to 6565";
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
                Data::handleNodeCallback(msg);
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

                        t = ptr_gist->canvasTwo;
                        ptr_gist->connectCanvas(t, ptr_gist->canvas);
                        qDebug() << "Switched to another canvas";
                    } 

                    t->reset(false); // no restarts
                    emit startWork();
                    qDebug() << ">>> no restarts";
                } else if (msg->restart_id == 0){

                    t->reset(true);
                    emit startWork();
                    qDebug() << ">>> new restart";
                } else {
                    qDebug() << ">>> restart and continue";
                }
            break;
            case DONE_SENDING:
                qDebug() << "Done receiving";
                updateCanvas();
                if (!Data::current->isRestarts())
                    Data::current->setDone();

            break;
        }

        if (t->refresh > 0 && nodeCount >= t->refresh) {
            t->currentNode->dirtyUp(*t->na);
            updateCanvas();
            emit statusChanged(false);
            nodeCount = 0;
            if (t->refreshPause > 0)
              msleep(t->refreshPause);
        }

    }

}

void
RecieverThread::updateCanvas(void) {
  qDebug() << "in RecieverThread::updateCanvas\n";
  // if (t == NULL) return; /// TODO: why do I need this all of a sudden?
  t->layoutMutex.lock();

    if (t->root == NULL) return;

    /// does this ever happen?
    if (t->autoHideFailed) {
        t->root->hideFailed(*t->na,true);
    }

    /// mark as dirty?
    for (VisualNode* n = t->currentNode; n != NULL; n=n->getParent(*t->na)) {
        if (n->isHidden()) {
            t->currentNode->setMarked(false);
            t->currentNode = n;
            t->currentNode->setMarked(true);
            break;
        }
    }
    

    t->root->layout(*t->na);
    BoundingBox bb = t->root->getBoundingBox();

    int w = static_cast<int>((bb.right-bb.left+Layout::extent)*t->scale);
    int h = static_cast<int>(2*Layout::extent+
                             t->root->getShape()->depth()
                             *Layout::dist_y*t->scale);
    t->xtrans = -bb.left+(Layout::extent / 2);

    int scale0 = static_cast<int>(t->scale*100);
    if (t->autoZoom) {
        QWidget* p = t->parentWidget();
        if (p) {
            double newXScale =
                    static_cast<double>(p->width()) / (bb.right - bb.left +
                                                       Layout::extent);
            double newYScale =
                    static_cast<double>(p->height()) /
                    (t->root->getShape()->depth() * Layout::dist_y + 2*Layout::extent);

            scale0 = static_cast<int>(std::min(newXScale, newYScale)*100);
            if (scale0<LayoutConfig::minScale)
                scale0 = LayoutConfig::minScale;
            if (scale0>LayoutConfig::maxAutoZoomScale)
                scale0 = LayoutConfig::maxAutoZoomScale;
            double scale = (static_cast<double>(scale0)) / 100.0;

            w = static_cast<int>((bb.right-bb.left+Layout::extent)*scale);
            h = static_cast<int>(2*Layout::extent+
                                 t->root->getShape()->depth()*Layout::dist_y*scale);
        }
    }

    t->layoutMutex.unlock();
    emit update(w,h,scale0);
}

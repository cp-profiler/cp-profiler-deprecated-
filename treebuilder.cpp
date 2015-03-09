#include "treebuilder.hh"
#include "debug.cpp"

TreeBuilder::TreeBuilder(TreeCanvas* tc, QObject *parent)
    : QThread(parent), _tc(tc) {
        _mutex = &(_tc->layoutMutex);
}

void TreeBuilder::startBuilding() {
    start();
}

void TreeBuilder::finishBuilding() {
    _data->setDone();
}

void TreeBuilder::reset(Data* data, NodeAllocator* na) {
	_data = data;
	_na = na;

    lastRead = 0;
}

void TreeBuilder::run(void) {
    
    clock_t begin, end;
    
    begin = clock();
	// qDebug() << "### in run method of tc:" << _tc->_id;

	std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
	std::unordered_map<unsigned long long, int> &sid2aid = _data->sid2aid;
	std::unordered_map<unsigned long long, int> &gid2aid = _data->gid2aid;

    bool _isRestarts = _data->isRestarts();

    VisualNode* node;
    VisualNode* parent;

    QMutex &dataMutex = _data->dataMutex;
    Statistics &stats = _tc->stats;

    stats.undetermined = 1;


    while(true) {

        int gid; /// gist Id
        unsigned long long pid;
        int parent_gid = -2; /// gist Id of parent
        int alt;
        int nalt;
        int status;

        int depth;

        while (!dataMutex.tryLock()) { 
            // qDebug() << "Can't lock, trying again";
        };

        // qDebug() << "lastRead:" << lastRead << "nodes_arr.size():" << nodes_arr.size();

        /// We can still be processing nodes when the DONE_SENDING message come
        /// so only stop when the nodes_arr is completely read
        if (lastRead >= nodes_arr.size()) {
            if (_data->isDone()) {
                qDebug() << "stop because done, " << "tc_id: " << _tc->_id;
                dataMutex.unlock();
                break;
            } else {
                dataMutex.unlock();
                continue;
            }
        }

        DbEntry& dbEntry = *nodes_arr[lastRead];

        // qDebug() << "node: " << dbEntry.parent_sid << " " << dbEntry.alt << " "
        //          << dbEntry.numberOfKids << " " << dbEntry.status;

        bool isRoot = (dbEntry.parent_sid == ~0u) ? true : false;

        // DebugHelper::printNodes(nodes_arr);
        // DebugHelper::printMap(sid2aid);

        _mutex->lock();

        if (isRoot) {
            if (_isRestarts) {
                int restart_root = (*_na)[0]->addChild(*_na); // create a node for a new root
                qDebug() << "restart_root_id: " << restart_root;
                VisualNode* new_root = (*_na)[restart_root];
                new_root->setNumberOfChildren(dbEntry.numberOfKids, *_na);
                new_root->setStatus(BRANCH);
                new_root->setHasSolvedChildren(true);
                new_root->_tid = dbEntry.thread;
                dbEntry.gid = restart_root;

            } else {
                int kids = nodes_arr[0]->numberOfKids;
                (*_na)[0]->setNumberOfChildren(kids, *_na);
                (*_na)[0]->setStatus(BRANCH);
                (*_na)[0]->setHasSolvedChildren(true);
                (*_na)[0]->_tid = 0; /// thread id
                dbEntry.gid = 0;
                stats.choices++;
                stats.undetermined += kids - 1;
                
            }
        }
        else { /// not a root

            pid = dbEntry.parent_sid;
            alt = dbEntry.alt;
            nalt = dbEntry.numberOfKids;
            status = dbEntry.status;
            parent_gid = nodes_arr[sid2aid[pid]]->gid;

            assert(parent_gid >= 0);

            parent = (*_na)[parent_gid];
            node = parent->getChild(*_na, alt);

            gid = node->getIndex(*_na);						// Gist ID
            dbEntry.gid = gid;

            gid2aid[gid] = lastRead;



//            qDebug() << "[" << lastRead << parent_gid << "] pid: " << pid << ", sid2aid:" << sid2aid[pid] << ", nodes_arr[sid2aid[pid]]->gid:" << nodes_arr[sid2aid[pid]]->gid;

            node->_tid = dbEntry.thread;
            node->setNumberOfChildren(nalt, *_na);

            // stats.maxDepth =
              // std::max(stats.maxDepth, depth);



            switch (status) {
                case FAILED: // 1
                    node->setHasOpenChildren(false);
                    node->setHasSolvedChildren(false);
                    node->setHasFailedChildren(true);
                    node->setStatus(FAILED);
                    parent->closeChild(*_na, true, false);
                    stats.failures++;
                    stats.undetermined--;

                break;
                case SKIPPED: // 6
                    node->setHasOpenChildren(false);
                    node->setHasSolvedChildren(false);
                    node->setHasFailedChildren(true);
                    node->setStatus(SKIPPED);
                    parent->closeChild(*_na, true, false);
                    stats.failures++;
                    stats.undetermined--;
                break;
                case SOLVED: // 0
                    node->setHasFailedChildren(false);
                    node->setHasSolvedChildren(true);
                    node->setHasOpenChildren(false);
                    node->setStatus(SOLVED);
                    parent->closeChild(*_na, false, true);
                    stats.solutions++;
                    stats.undetermined--;
                break;
                case BRANCH: // 2
                    node->setHasOpenChildren(true);
                    node->setStatus(BRANCH);
                    stats.choices++;
                    stats.undetermined += nalt - 1;
                break;
                default:
                    qDebug() << "need to handle this type of Node: " << status;
                break;

            }
                
            static_cast<VisualNode*>(node)->changedStatus(*_na);
            node->dirtyUp(*_na);

        }

        _mutex->unlock();

        dataMutex.unlock();

        lastRead++;

    }

    node->dirtyUp(*_na);
    emit doneBuilding();
    end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
     qDebug() << "Time elapsed: " << elapsed_secs << " seconds";

}

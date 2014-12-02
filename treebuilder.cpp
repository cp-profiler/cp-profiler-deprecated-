#include "treebuilder.hh"

TreeBuilder::TreeBuilder(TreeCanvas* tc, QObject *parent)
    : QThread(parent), _tc(tc) {
        _mutex = &(_tc->layoutMutex);
        qDebug() << "+++ new Tree Builder, tc_id: " << _tc->_id;
}

void TreeBuilder::startBuilding() {
    qDebug() << ">>> startBuilding in Builder #" << _tc->_id;
    start();
}

void TreeBuilder::reset(Data* data, NodeAllocator* na) {
	_data = data;
	_na = na;

    lastRead = 0;
}

void TreeBuilder::run(void) {
    
    clock_t begin, end;

    bool showlocks = false;
    
    begin = clock();
	qDebug() << "### in run method of tc:" << _tc->_id;

	std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
	std::unordered_map<unsigned long long, int> &sid2aid = _data->sid2aid;
	std::unordered_map<unsigned long long, int> &gid2aid = _data->gid2aid;

    bool _isRestarts = _data->isRestarts();

    VisualNode* node;
    VisualNode* parent;

    QMutex &dataMutex = _data->dataMutex;
    Statistics &stats = _tc->stats;

    while(true) {

        int gid; /// gist Id
        unsigned long long pid;
        int parent_gid = -2; /// gist Id of parent
        int alt;
        int nalt;
        int status;


        if (showlocks) qDebug() << "lock mutex 51";
        while (!dataMutex.tryLock()) { 
            // qDebug() << "Can't lock, trying again";
        };


        if (lastRead >= nodes_arr.size()) {
            if (_data->isDone()) {
                qDebug() << "stop because done";
                dataMutex.unlock();
                if (showlocks) qDebug() << "unlock mutex 58";
                break;
            } else {
                if (showlocks) qDebug() << "unlock  mutex 61";
                dataMutex.unlock();
                continue;
            }
        }

        DbEntry& dbEntry = *nodes_arr[lastRead];


        bool isRoot = (dbEntry.parent_sid == ~0u) ? true : false;


//        qDebug() << "gid: " << dbEntry.gid << "parent_sid: " << dbEntry.parent_sid << dbEntry.alt << dbEntry.alt;

        if (showlocks) qDebug() << "lock mutex 72";
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
                (*_na)[0]->setNumberOfChildren(nodes_arr[0]->numberOfKids, *_na);
                (*_na)[0]->setStatus(BRANCH);
                (*_na)[0]->setHasSolvedChildren(true);
                (*_na)[0]->_tid = 0; /// thread id
                dbEntry.gid = 0;
            }  
        }
        else { /// not a root

            pid = dbEntry.parent_sid;
            // show_db();

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

            // qDebug() << "[" << lastRead << parent_gid << "] pid: " << pid << ", sid2aid:" << sid2aid[pid] << ", nodes_arr[sid2aid[pid]]->gid:" << nodes_arr[sid2aid[pid]]->gid;

            node->_tid = dbEntry.thread;
            node->setNumberOfChildren(nalt, *_na);

            switch (status) {
                case FAILED: // 1
                    node->setHasOpenChildren(false);
                    node->setHasSolvedChildren(false);
                    node->setHasFailedChildren(true);
                    node->setStatus(FAILED);
                    parent->closeChild(*_na, true, false);
                    stats.failures++;
                break;
                case SKIPPED: // 6
                    node->setHasOpenChildren(false);
                    node->setHasSolvedChildren(false);
                    node->setHasFailedChildren(true);
                    node->setStatus(SKIPPED);
                    parent->closeChild(*_na, true, false);
                    stats.failures++;
                break;
                case SOLVED: // 0
                    node->setHasFailedChildren(false);
                    node->setHasSolvedChildren(true);
                    node->setHasOpenChildren(false);
                    node->setStatus(SOLVED);
                    parent->closeChild(*_na, false, true);
                    stats.solutions++;
                break;
                case BRANCH: // 2
                    node->setHasOpenChildren(true);
                    node->setStatus(BRANCH);
                    stats.choices++;
                break;

            }
                
            static_cast<VisualNode*>(node)->changedStatus(*_na);
            node->dirtyUp(*_na);

        }

        _mutex->unlock();
        // qDebug() << "mutext unlock in run";

        if (showlocks) qDebug() << "unlock mutex 157";
        dataMutex.unlock();

        lastRead++;

    }

    // qDebug() << "lastRead: " << lastRead;
    node->dirtyUp(*_na);
    emit doneBuilding();
    // qDebug() << "Done building";
    end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
     qDebug() << "Time elapsed: " << elapsed_secs << " seconds";

}

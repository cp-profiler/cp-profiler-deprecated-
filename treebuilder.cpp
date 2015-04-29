#include "treebuilder.hh"
#include "debug.cpp"

TreeBuilder::TreeBuilder(TreeCanvas* tc, QObject *parent)
    : QThread(parent), _tc(tc) {
        _mutex = &(_tc->layoutMutex);
}

void TreeBuilder::startBuilding() {
    start();
}

void TreeBuilder::setDoneReceiving() {
    _data->setDoneReceiving();
}

void TreeBuilder::reset(Data* data, NodeAllocator* na) {

    /// old _data and _na are deleted by this point
	_data = data;
	_na = na;

    nodesCreated = 1;
    lastRead = 0;
}

void TreeBuilder::processRoot(DbEntry& dbEntry) {

    std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
    std::vector<unsigned long long> &gid2aid = _data->gid2aid;
    Statistics &stats = _tc->stats;

    stats.choices++;

    VisualNode* root = nullptr; // can be a real root, or one of initial nodes in restarts

    int kids = dbEntry.numberOfKids;

    if (_data->isRestarts()) {
        int restart_root = (*_na)[0]->addChild(*_na); // create a node for a new root        
        root = (*_na)[restart_root];
        root->_tid = dbEntry.thread;
        dbEntry.gid = restart_root;
        dbEntry.depth = 2;

    } else {
        root = (*_na)[0]; // use the root that is already there
        root->_tid = 0;
        dbEntry.gid = 0;
        dbEntry.depth = 1;
    }

    /// setNumberOfChildren
    root->setNumberOfChildren(kids, *_na);
    root->setStatus(BRANCH);
    root->setHasSolvedChildren(true);

    stats.undetermined += kids - 1;
    nodesCreated += 1 + kids;

    gid2aid.resize(nodesCreated, -1);
}

void TreeBuilder::processNode(DbEntry& dbEntry) {
    unsigned long long pid = dbEntry.parent_sid; /// parent ID as it comes from Solver
    int alt      = dbEntry.alt;        /// which alternative the current node is
    int nalt     = dbEntry.numberOfKids; /// number of kids in current node
    int status   = dbEntry.status;

    std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
    std::unordered_map<unsigned long long, int> &sid2aid = _data->sid2aid;
    std::vector<unsigned long long> &gid2aid = _data->gid2aid;

    Statistics &stats = _tc->stats;

    DbEntry& parentEntry = *nodes_arr[sid2aid[pid]];
    int parent_gid  = parentEntry.gid; /// parent ID as it is in Node Allocator (Gist)
    VisualNode& parent = *(*_na)[parent_gid];

    assert(parent_gid >= 0);

    VisualNode& node = *parent.getChild(*_na, alt);

    /// !!!!!!!!!!!! Tomorrow: Fix skipped node overriding

    if (node.getStatus() == UNDETERMINED) {
        stats.undetermined--;

        int gid = node.getIndex(*_na); // node ID as it is in Gist

        /// fill in empty fields of dbEntry
        dbEntry.gid = gid;
        dbEntry.depth   = parentEntry.depth + 1; /// parent's depth + 1
        gid2aid[gid] = lastRead; /// this way we can find dbEntry

        stats.maxDepth =
          std::max(stats.maxDepth, static_cast<int>(dbEntry.depth));

        node._tid = dbEntry.thread; /// TODO: tid should be in node's flags
        node.setNumberOfChildren(nalt, *_na);

        switch (status) {
            case FAILED: // 1
                node.setHasOpenChildren(false);
                node.setHasSolvedChildren(false);
                node.setHasFailedChildren(true);
                node.setStatus(FAILED);
                parent.closeChild(*_na, true, false);
                stats.failures++;

            break;
            case SKIPPED: // 6
                // check if node hasn't been explored by other thread
                // (for now just check if failure node)
                node.setHasOpenChildren(false);
                node.setHasSolvedChildren(false);
                node.setHasFailedChildren(true);
                node.setStatus(SKIPPED);
                parent.closeChild(*_na, true, false);
                stats.failures++;
            break;
            case SOLVED: // 0
                node.setHasFailedChildren(false);
                node.setHasSolvedChildren(true);
                node.setHasOpenChildren(false);
                node.setStatus(SOLVED);
                parent.closeChild(*_na, false, true);
                stats.solutions++;
            break;
            case BRANCH: // 2
                node.setHasOpenChildren(true);
                node.setStatus(BRANCH);
                stats.choices++;
                stats.undetermined += nalt;
                nodesCreated += nalt;
                gid2aid.resize(nodesCreated, -1);
            break;
            default:
                qDebug() << "need to handle this type of Node: " << node.getStatus();
            break;

        }

        node.changedStatus(*_na);
        node.dirtyUp(*_na);
    }
}


void TreeBuilder::run(void) {
    
    clock_t begin, end;
    
    begin = clock();
	// qDebug() << "### in run method of tc:" << _tc->_id;

    bool _isRestarts = _data->isRestarts();

    std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
    QMutex &dataMutex = _data->dataMutex;
    
    Statistics &stats = _tc->stats;
    stats.undetermined = 1;

    while(true) {

        system_clock::time_point before_tp = system_clock::now();

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
            processRoot(dbEntry);            
        } else { /// not a root
            processNode(dbEntry);
        }

           

        _mutex->unlock();

        dataMutex.unlock();

        lastRead++;

        // system_clock::time_point after_tp = system_clock::now();
        // qDebug () << "building node takes: " << 
        //     duration_cast<nanoseconds>(after_tp - before_tp).count() << "ns";

    }

    emit doneBuilding(true);

    end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    qDebug() << "Time elapsed: " << elapsed_secs << " seconds";

    qDebug() << "solutions:" << stats.solutions;
    qDebug() << "failures:" << stats.failures;
    qDebug() << "undetermined:" << stats.undetermined;

    /// test if first argument is '--test'
    if (qApp->arguments().size() > 1 && qApp->arguments().at(1) == "--test")
        qApp->exit();

}

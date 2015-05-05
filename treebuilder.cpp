#include "treebuilder.hh"
#include "debug.cpp"
#include "readingQueue.hh"
#include <cassert>

TreeBuilder::TreeBuilder(TreeCanvas* tc, QObject *parent)
    : QThread(parent), _tc(tc), read_queue(nullptr) {
        layout_mutex = &(_tc->layoutMutex);
}

TreeBuilder::~TreeBuilder() {
    delete read_queue;
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

    delete read_queue;
    read_queue = new ReadingQueue(data->nodes_arr);

    nodesCreated = 1;
    lastRead = 0;
}

bool TreeBuilder::processRoot(DbEntry& dbEntry) {

    QMutexLocker locker(layout_mutex);

    std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
    auto& gid2entry = _data->gid2entry;

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

    gid2entry[dbEntry.gid] = &dbEntry;

    /// setNumberOfChildren
    root->setNumberOfChildren(kids, *_na);
    root->setStatus(BRANCH);
    root->setHasSolvedChildren(true);

    stats.undetermined += kids - 1;
    nodesCreated += 1 + kids;

    root->changedStatus(*_na);
    root->dirtyUp(*_na);

    return true;
}

bool TreeBuilder::processNode(DbEntry& dbEntry, bool is_delayed) {
    QMutexLocker locker(layout_mutex);

    unsigned long long pid = dbEntry.parent_sid; /// parent ID as it comes from Solver
    int alt      = dbEntry.alt;        /// which alternative the current node is
    int nalt     = dbEntry.numberOfKids; /// number of kids in current node
    int status   = dbEntry.status;

    std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
    std::unordered_map<unsigned long long, int> &sid2aid = _data->sid2aid;
    auto& gid2entry = _data->gid2entry;

    Statistics &stats = _tc->stats;


    /// find out if node exists

    auto pid_it = sid2aid.find(pid);
    std::cerr << "process node: " << dbEntry << "\n";


    if (pid_it == sid2aid.end()) {
        qDebug() << "node for parent is not in db yet";

        if (!is_delayed)
            read_queue->readLater(&dbEntry);
        else
            qDebug() << "node already in the queue";

        return false;
    }

    std::cerr << "sid2aid[pid]: " << pid_it->second << "\n";
    

    DbEntry& parentEntry = *nodes_arr[pid_it->second];
    int parent_gid  = parentEntry.gid; /// parent ID as it is in Node Allocator (Gist)
    VisualNode& parent = *(*_na)[parent_gid];

    // assert(parent_gid >= 0);
    if (parent_gid < 0) {
        // qDebug() << "Ignoring a node: " << ignored_entries.size();
        ignored_entries.push_back(&dbEntry);
        return false;
    }

    VisualNode& node = *parent.getChild(*_na, alt);

    /// Normal behaviour: insert into Undetermined

    if (node.getStatus() == UNDETERMINED) {
        stats.undetermined--;

        int gid = node.getIndex(*_na); // node ID as it is in Gist

        /// fill in empty fields of dbEntry
        dbEntry.gid = gid;
        dbEntry.depth   = parentEntry.depth + 1; /// parent's depth + 1
        gid2entry[gid] = &dbEntry;

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
            break;
            default:
                qDebug() << "need to handle this type of Node: " << status;
            break;

        }

        node.changedStatus(*_na);
        node.dirtyUp(*_na);
    } else {
        /// Not normal cases:
        /// 1. Branch Node or Failed node into Skipped

        if (node.getStatus() == SKIPPED) {
            switch (status) {
                case FAILED: // 1
                    node.setHasOpenChildren(false);
                    node.setHasSolvedChildren(false);
                    node.setHasFailedChildren(true);
                    node.setStatus(FAILED);
                    parent.closeChild(*_na, true, false);
                    stats.failures++;

                break;
                case BRANCH: // 2
                    node.setHasOpenChildren(true);
                    node.setStatus(BRANCH);
                    stats.choices++;
                    stats.undetermined += nalt;
                    nodesCreated += nalt;
                break;
                default:
                    qDebug() << "need to handle this type of Node: " << status;
                    assert(status != SOLVED);
                break;
            }
        } else {
            // qDebug() << "Ignoring a node: " << ignored_entries.size();
            // assert(status == SKIPPED);
            ignored_entries.push_back(&dbEntry);
            /// sometimes branch wants to override branch
        }



    }

    return true;
}


void TreeBuilder::run(void) {

    using std::cout; using std::cerr;
    
    clock_t begin, end;
    
    begin = clock();
	// qDebug() << "### in run method of tc:" << _tc->_id;

    bool _isRestarts = _data->isRestarts();

    std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
    QMutex &dataMutex = _data->dataMutex;
    
    Statistics &stats = _tc->stats;
    stats.undetermined = 1;

    bool is_delayed;

    while(true) {

        system_clock::time_point before_tp = system_clock::now();

        int depth;

        while (!dataMutex.tryLock()) { /* qDebug() << "Can't lock, trying again"; */ };


        /// check if done
        if (!read_queue->canRead()) {
            dataMutex.unlock();

            if (_data->isDone()) {
                qDebug() << "stop because done " << "tc_id: " << _tc->_id;
                break;
            }
            /// can't read, but receiving not done, waiting...
            msleep(1);
            continue;
        }

        /// ask queue for an entry
        DbEntry* entry = read_queue->next(is_delayed);

        std::cout << "about to process: " << *entry << "\n";

        bool isRoot = (entry->parent_sid == ~0u) ? true : false;

        /// TODO: process node should know if the node is delayed

        /// try to put node into the tree
        bool success = isRoot ? processRoot(*entry) : processNode(*entry, is_delayed);

        read_queue->update(success);

        dataMutex.unlock();


        system_clock::time_point after_tp = system_clock::now();
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

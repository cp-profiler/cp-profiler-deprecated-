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


#include "treebuilder.hh"
#include "globalhelper.hh"
#include "readingQueue.hh"
#include "treecanvas.hh"
#include <cassert>

#include <time.h>
#include <sys/time.h>

double get_wall_time(){
    struct timeval time;
    if (gettimeofday(&time,nullptr)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

TreeBuilder::TreeBuilder(TreeCanvas* tc, QObject *parent)
    : QThread(parent), _tc(tc), read_queue(nullptr) {
        layout_mutex = &(_tc->layoutMutex);
        connect(this, &TreeBuilder::doneBuilding, tc->getExecution(), &Execution::doneBuilding);
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

void TreeBuilder::reset(Execution* execution, NodeAllocator* na) {

    /// old _data and _na are deleted by this point (where?)
    _data = execution->getData();
	_na = na;

    delete read_queue;
    read_queue = new ReadingQueue(_data->nodes_arr);

    nodesCreated = 1;
    lastRead = 0;

    /// TODO(maxim): is this needed?
    // connect(this, &TreeBuilder::doneBuilding, execution, &Execution::doneBuilding);
}

bool TreeBuilder::processRoot(DbEntry& dbEntry) {
    QMutexLocker locker(layout_mutex);

    std::cerr << "process root: " << dbEntry << "\n";

    auto& gid2entry = _data->gid2entry;

    Statistics &stats = _tc->stats;

    stats.choices++;

    VisualNode* root = nullptr; // can be a real root, or one of initial nodes in restarts

    int kids = dbEntry.numberOfKids;

    if (_data->isRestarts()) {
        int restart_root = (*_na)[0]->addChild(*_na); // create a node for a new root
        root = (*_na)[restart_root];
        root->_tid = dbEntry.thread_id;
        dbEntry.gid = restart_root;
        dbEntry.depth = 2;
    } else {
        root = (*_na)[0]; // use the root that is already there
        root->_tid = 0;
        dbEntry.gid = 0;
        dbEntry.depth = 1;
    }
    dbEntry.decisionLevel = 0;

    gid2entry[dbEntry.gid] = &dbEntry;

    /// setNumberOfChildren
    root->setNumberOfChildren(kids, *_na);
    root->setStatus(BRANCH);
    root->setHasSolvedChildren(false);
    root->setHasOpenChildren(true);

    stats.undetermined += kids - 1;
    nodesCreated += 1 + kids;

    root->changedStatus(*_na);
    root->dirtyUp(*_na);

    emit addedNode();

    return true;
}

bool TreeBuilder::processNode(DbEntry& dbEntry, bool is_delayed) {
    // std::cerr << "TreeBuilder::processNode (" << dbEntry << ")\n";
    QMutexLocker locker(layout_mutex);

    int64_t pid = dbEntry.parent_sid; /// parent ID as it comes from Solver
    int alt      = dbEntry.alt;        /// which alternative the current node is
    int nalt     = dbEntry.numberOfKids; /// number of kids in current node
    int status   = dbEntry.status;

    std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
    std::unordered_map<int64_t, int> &sid2aid = _data->sid2aid;
    auto& gid2entry = _data->gid2entry;

    Statistics &stats = _tc->stats;


    /// find out if node exists

    auto pid_it = sid2aid.find(pid);
    // std::cerr << "process node: " << dbEntry << "\n";

    if (pid_it == sid2aid.end()) {
        // qDebug() << "node for parent is not in db yet";

        if (!is_delayed)
            read_queue->readLater(&dbEntry);

        return false;
    }

    // std::cerr << "sid2aid[pid]: " << pid_it->second << "\n";

    DbEntry& parentEntry = *nodes_arr[pid_it->second];
    int parent_gid  = parentEntry.gid; /// parent ID as it is in Node Allocator (Gist)

    /// put delayed also if parent node hasn't been processed yet:
    if (parent_gid == -1) {
        // qDebug() << "parent arrived, but has not been processed yet";

        if (!is_delayed)
            read_queue->readLater(&dbEntry);
        else
            qDebug() << "node already in the queue";

        return false;
    }


    VisualNode& parent = *(*_na)[parent_gid];

    assert(parent_gid >= 0);
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

        // It is not always clear how to compute a node's decision
        // level with respect to its parent.  For now, let us say that
        // the right-most branch is not a decision, and all other
        // branches are decisions.
        bool thisIsRightmost = (alt == parentEntry.numberOfKids-1);
        dbEntry.decisionLevel = parentEntry.decisionLevel +
            (thisIsRightmost ? 0 : 1);

        gid2entry[gid] = &dbEntry;

        stats.maxDepth =
          std::max(stats.maxDepth, static_cast<int>(dbEntry.depth));

        node._tid = dbEntry.thread_id; /// TODO: tid should be in node's flags
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
        emit addedNode();
        // std::cerr << "TreeBuilder::processNode, normal case\n";
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
        node.changedStatus(*_na);
        node.dirtyUp(*_na);
        emit addedNode();
        // std::cerr << "TreeBuilder::processNode, not-normal case\n";
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

    std::cerr << "TreeBuilder::run\n";

    clock_t beginClock, endClock;
    double beginTime, endTime;

    beginClock = clock();
    beginTime = get_wall_time();
	// qDebug() << "### in run method of tc:" << _tc->_id;

    QMutex &dataMutex = _data->dataMutex;

    Statistics &stats = _tc->stats;
    stats.undetermined = 1;

    bool is_delayed;

    while(true) {

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

        /// ask queue for an entry, note: is_delayed gets assigned here
        DbEntry* entry = read_queue->next(is_delayed);

        bool isRoot = (entry->parent_sid == -1) ? true : false;

        /// try to put node into the tree
        bool success = isRoot ? processRoot(*entry) : processNode(*entry, is_delayed);

        read_queue->update(success);

        dataMutex.unlock();

    }

    emit doneBuilding(true);

    endClock = clock();
    endTime = get_wall_time();

    double elapsed_clock_secs = double(endClock - beginClock) / CLOCKS_PER_SEC;
    //    qDebug() << "Time elapsed: " << elapsed_secs << " seconds";
    qDebug() << "Elapsed CPU time:  " << elapsed_clock_secs << " seconds";
    qDebug() << "Elapsed wall time: " << (endTime - beginTime) << " seconds";
    // qDebug() << fixed << beginTime << "  ->  " << endTime;

    qDebug() << "solutions:" << stats.solutions;
    qDebug() << "failures:" << stats.failures;
    qDebug() << "undetermined:" << stats.undetermined;

    /// test if first argument is '--test'
    // if (qApp->arguments().size() > 1 && qApp->arguments().at(1) == "--test")
    //     qApp->exit();


    if (GlobalParser::isSet(GlobalParser::test_option)) {
        qDebug() << "test mode, terminate";
        qApp->exit();
    }

}

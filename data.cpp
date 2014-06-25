#include <iostream>
#include <qdebug.h>
#include <string>
#include <sstream>

#include <ctime>
#include <zmq.hpp>


#include "visualnode.hh"
#include "data.hh"

using namespace std;

Data* Data::self = 0;

Data::Data(TreeCanvas* tc, NodeAllocator* na, bool isRestarts)
 : _tc(tc), _na(na), _isRestarts(isRestarts) {
    Data::self = this;

    lastRead = -1;
}


void Data::show_db(void) {
    qDebug() << "***** SHOW_DB: *****";
    for (vector<DbEntry*>::iterator it = nodes_arr.begin(); it != nodes_arr.end(); it++) {
            qDebug() << "sid: " << (*it)->gid << " p: " << (*it)->parent_sid <<
            " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
    qDebug() << "***** _________________________ *****";
}

/// return false if there is nothing to read
bool Data::readInstance(bool isRoot) {
    int sid; /// solver Id
    int gid; /// gist Id
    unsigned long long pid;
    int parent_gid; /// gist Id of parent
    int alt;
    int nalt;
    int status;

    if (lastRead +1 >= nodes_arr.size())
        return false;

    VisualNode* node;
    VisualNode* parent;

    if (isRoot) { 
        lastRead++;
        if (_isRestarts) {
            int restart_root = (*_na)[0]->addChild(*_na); // create a node for a new root
            qDebug() << "restart_root_id: " << restart_root;
            VisualNode* new_root = (*_na)[restart_root];
            new_root->setNumberOfChildren(nodes_arr[lastRead]->numberOfKids, *_na);
            new_root->setStatus(BRANCH);
            new_root->setHasSolvedChildren(true);
            new_root->_tid = nodes_arr[lastRead]->thread;
            nodes_arr[lastRead]->gid = restart_root;

        } else {
            (*_na)[0]->setNumberOfChildren(nodes_arr[0]->numberOfKids, *_na);
            (*_na)[0]->setStatus(BRANCH);
            (*_na)[0]->setHasSolvedChildren(true);
            (*_na)[0]->_tid = 0; /// thread id
            nodes_arr[lastRead]->gid = 0;
        }  
    }
    else {

        lastRead++;

        pid = nodes_arr[lastRead]->parent_sid;
        // show_db();
        qDebug() << "last read: " << lastRead;
        qDebug() << "parent_sid: " << pid;

        alt = nodes_arr[lastRead]->alt;
        nalt = nodes_arr[lastRead]->numberOfKids;
        status = nodes_arr[lastRead]->status;
        parent_gid = nodes_arr[sid2aid[pid]]->gid;
        parent = (*_na)[parent_gid];
        node = parent->getChild(*_na, alt);
        gid = node->getIndex(*_na);
        nodes_arr[lastRead]->gid = gid;
        gid2aid[gid] = lastRead;


        node->_tid = nodes_arr[lastRead]->thread;
        node->setNumberOfChildren(nalt, *_na);

        switch (status) {
            case FAILED: // 1
                node->setHasOpenChildren(false);
                node->setHasSolvedChildren(false);
                node->setHasFailedChildren(true);
                node->setStatus(FAILED);
                parent->closeChild(*_na, true, false);
                _tc->stats.failures++;
            break;
            case SKIPPED: // 6
                node->setHasOpenChildren(false);
                node->setHasSolvedChildren(false);
                node->setHasFailedChildren(true);
                node->setStatus(SKIPPED);
                parent->closeChild(*_na, true, false);
                _tc->stats.failures++;
            break;
            case SOLVED: // 0
                node->setHasFailedChildren(false);
                node->setHasSolvedChildren(true);
                node->setHasOpenChildren(false);
                node->setStatus(SOLVED);
                parent->closeChild(*_na, false, true);
                _tc->stats.solutions++;
            break;
            case BRANCH: // 2
                node->setHasOpenChildren(true);
                node->setStatus(BRANCH);
                _tc->stats.choices++;
            break;

        }

        // node->setStatus(NodeStatus(status));
        // statusChanged(false); // what does this do?
            
        static_cast<VisualNode*>(node)->changedStatus(*_na);
        node->dirtyUp(*_na);

    }

    return true;

}

void Data::startReading(void) {
    qDebug() << "in startReading...\n";

}

int Data::handleNodeCallback(Message* msg) {
    int id, pid, alt, kids, status, restart_id;
    char thread;

    bool isRoot = false;

    Data& data = *Data::self;
    
    id = msg->sid;
    pid = msg->parent_sid;
    alt = msg->alt;
    kids = msg->kids;
    status = msg->status;
    thread = msg->thread;
    restart_id = msg->restart_id;

    qDebug() << "Received node: \t" << id << " " << pid << " "
                    << alt << " " << kids << " " << status << " wid: "
                    << (int)thread << " restart: " << restart_id;    

    if (pid == -1) {
        pid = 0; /// check if still needed
        isRoot = true;
    }

    BigId real_id = id | ((long long)restart_id << 32);
    BigId real_pid = pid | ((long long)restart_id << 32);

    Data::self->pushInstance(real_id.value() - data.firstIndex,
        new DbEntry(real_pid.value() - data.firstIndex, alt, kids, thread, msg->label, status));

    // qDebug() << "Pushed node: \t" << real_id.value() - data.firstIndex << " "
    //     << real_pid.value() - data.firstIndex << " " << alt << " " << kids << " "
    //     << status << " wid: " << (int)msg->thread;

    if (pid != -1) data.lastArrived = id;

    data.readInstance(isRoot);

    return 0;
}

void Data::pushInstance(unsigned long long sid, DbEntry* entry) {

    sid2aid[sid] = nodes_arr.size();

    nodes_arr.push_back(entry);
}

char* Data::getLabelByGid(unsigned int gid) {
    return nodes_arr[ gid2aid[gid] ]->label;
}


Data::~Data(void) {
    //sqlite3_close(db);

    for (vector<DbEntry*>::iterator it = nodes_arr.begin(); it != nodes_arr.end(); it++) {
        delete (*it);
    }
}




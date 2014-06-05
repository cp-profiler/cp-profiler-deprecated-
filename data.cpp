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

Data::Data(TreeCanvas* tc, NodeAllocator* na) : _tc(tc), _na(na) {
    Data::self = this;
    counter = 0;

    lastRead = -1;
}


void Data::show_db(void) {
    for (vector<DbEntry*>::iterator it = nodes_arr.begin(); it != nodes_arr.end(); it++) {
            qDebug() << "sid: " << (*it)->gid << " p: " << (*it)->parent_sid <<
            " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
}

/// return false if there is nothing to read
bool Data::readInstance(NodeAllocator *na) {
    int sid; /// solver Id
    int gid; /// gist Id
    int pid;
    int parent_gid; /// gist Id of parent
    int alt;
    int nalt;
    int status;

    if (lastRead +1 >= counter)
        return false;

    VisualNode* node;
    VisualNode* parent;

    if (lastRead == -1) {
        lastRead++;
        (*na)[0]->setNumberOfChildren(nodes_arr[0]->numberOfKids, *na);
        (*na)[0]->setStatus(BRANCH);
        (*na)[0]->setHasSolvedChildren(true);
        (*na)[0]->_tid = 0;
        nodes_arr[lastRead]->gid = 0;
    }
    else {

        lastRead++;

        pid = nodes_arr[lastRead]->parent_sid;
        alt = nodes_arr[lastRead]->alt;
        nalt = nodes_arr[lastRead]->numberOfKids;
        status = nodes_arr[lastRead]->status;
        parent_gid = nodes_arr[sid2aid[pid]]->gid;
        parent = (*na)[parent_gid];
        node = parent->getChild(*na, alt);
        gid = node->getIndex(*na);
        nodes_arr[lastRead]->gid = gid;
        gid2aid[gid] = lastRead;

        // qDebug() << "(*na)[" << nodes_arr[lastRead]->gid << "]: " << parent_gid;
             
        // qDebug() << "lastRead: " << lastRead; 
        // qDebug() << "parent gist id: " << parent_gid;
        // qDebug() << "parent id: " << pid;
        // qDebug() << "sid2aid[" << pid << "]: " << sid2aid[pid];
        // qDebug() << "nodes_arr[" << sid2aid[pid] << "]: " << nodes_arr[sid2aid[pid]]->gid;


        node->_tid = nodes_arr[lastRead]->thread;
        node->setNumberOfChildren(nalt, *na);

        switch (status) {
            case FAILED: // 1
                node->setHasOpenChildren(false);
                node->setHasSolvedChildren(false);
                node->setHasFailedChildren(true);
                node->setStatus(FAILED);
                parent->closeChild(*na, true, false);
                _tc->stats.failures++;
            break;
            case SKIPPED: // 6
                node->setHasOpenChildren(false);
                node->setHasSolvedChildren(false);
                node->setHasFailedChildren(true);
                node->setStatus(SKIPPED);
                parent->closeChild(*na, true, false);
                _tc->stats.failures++;
            break;
            case SOLVED: // 0
                node->setHasFailedChildren(false);
                node->setHasSolvedChildren(true);
                node->setHasOpenChildren(false);
                node->setStatus(SOLVED);
                parent->closeChild(*na, false, true);
                _tc->stats.solutions++;
            break;
            case BRANCH: // 2
                node->setHasOpenChildren(true);
                node->setStatus(BRANCH);
                _tc->stats.choices++;
            break;

        }

        // node->setStatus(NodeStatus(status));
            
        static_cast<VisualNode*>(node)->changedStatus(*na);
        node->dirtyUp(*na);
        _tc->statusChanged(false);

    }

    return true;

}

void Data::startReading(void) {
    qDebug() << "in startReading...\n";

}

int Data::handleNodeCallback(Message* data) {
    int id, parent, alt, kids, status;
    char thread;
    
    id = data->sid;
    parent = data->parent_sid;
    alt = data->alt;
    kids = data->kids;
    status = data->status;
    thread = data->thread;

    std::cout << "Received node: " << id << " " << parent << " "
                    << alt << " " << kids << " " << status << " wid: "
                    << (int)data->thread << std::endl;

    Data::self->pushInstance(id - Data::self->firstIndex,
        new DbEntry(parent - Data::self->firstIndex, alt, kids, thread, data->label, status));
    Data::self->counter++;

    Data::self->readInstance(Data::self->_na);

    return 0;
}

void Data::pushInstance(unsigned int sid, DbEntry* entry) {
    // qDebug() << "sid2aid[" << sid << "] = " << counter;
    sid2aid[sid] = counter;
    // qDebug() << "status: " << entry->status;
    
    // if (entry->alt > 1)
    //     qDebug() << "alt = 2, really?";
    // if (entry->alt == 0)
    //     qDebug() << "alt = 0, ok";
    // else if (entry->alt == 1)
    //     qDebug() << "alt = 1, ok";
    // qDebug() << "sid2aid[" << sid << "]: " << counter; 

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




#include <iostream>
#include <qdebug>
#include <string>
#include <sstream>

#include <ctime>
#include <zmq.hpp>


#include "visualnode.hh"
#include "data.hh"

using namespace std;

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
                ( std::ostringstream() << std::dec << x ) ).str()

Data* Data::self = 0;

Data::Data(TreeCanvas* tc, NodeAllocator* na) : _tc(tc), _na(na) {
    Data::self = this;
    counter = 0;

    lastRead = -1;

}


void Data::show_db(void) {
    for (vector<DbEntry*>::iterator it = db_array.begin(); it != db_array.end(); it++) {
            qDebug() << "nid: " << (*it)->node_id << " p: " << (*it)->parent_db_id <<
            " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
}

/// return false if there is nothing to read
bool Data::readInstance(NodeAllocator *na) {
    int nid;
    int parent_db_id;
    int parent_nid;
    int alt;
    int nalt;
    int status;

    if (lastRead +1 >= counter)
        return false;

    VisualNode* node;
    VisualNode* parent;

    if (lastRead == -1) {
        lastRead++;
        (*na)[0]->setNumberOfChildren(db_array[0]->numberOfKids, *na);
        (*na)[0]->setStatus(BRANCH);
        (*na)[0]->setHasSolvedChildren(true);
        db_array[lastRead]->node_id = 0;
    }
    else {
        lastRead++;
        parent_db_id = db_array[lastRead]->parent_db_id;
        alt = db_array[lastRead]->alt;
        nalt = db_array[lastRead]->numberOfKids;
        status = db_array[lastRead]->status;
        parent_nid = db_array[parent_db_id]->node_id;
        parent = (*na)[parent_nid];
        node = parent->getChild(*na, alt);
        db_array[lastRead]->node_id = node->getIndex(*na);
        node->setNumberOfChildren(nalt, *na);

        switch (status) {
            case FAILED: // 1
                node->setHasOpenChildren(false);
                node->setHasSolvedChildren(false);
                node->setHasFailedChildren(true);
                parent->closeChild(*na, true, false);
                node->setStatus(FAILED);
                _tc->stats.failures++;
            break;
            case SOLVED: // 0
                node->setHasFailedChildren(false);
                node->setHasSolvedChildren(true);
                node->setHasOpenChildren(false);
                parent->closeChild(*na, false, true);
                node->setStatus(SOLVED);
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
    Data::self->counter++;
    id = data->sid;
    parent = data->parent;
    alt = data->alt;
    kids = data->kids;
    status = data->status;


    /// should probably combine this with the one in readInstance(...)
    if (Data::self->lastRead == -1) {
        Data::self->firstIndex = id;
    }

    Data::self->db_array.push_back(new DbEntry(parent - Data::self->firstIndex, alt, kids, status));

    Data::self->readInstance(Data::self->_na);

    return 0;
}


Data::~Data(void) {
    sqlite3_close(db);

    for (vector<DbEntry*>::iterator it = db_array.begin(); it != db_array.end(); it++) {
        delete (*it);
    }
}




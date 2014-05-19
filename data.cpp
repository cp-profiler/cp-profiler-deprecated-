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
            qDebug() << "nid: " << (*it)->node_id << " p: " << (*it)->parent_id <<
            " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
}

/// return false if there is nothing to read
bool Data::readInstance(NodeAllocator *na) {
    int nid;
    int pid;
    int parent_gist_id;
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

        pid = db_array[lastRead]->parent_id;
        alt = db_array[lastRead]->alt;
        nalt = db_array[lastRead]->numberOfKids;
        status = db_array[lastRead]->status;
        parent_gist_id = db_array[nid_to_db_id[pid]]->node_id;
        parent = (*na)[parent_gist_id];
        node = parent->getChild(*na, alt);
        db_array[lastRead]->node_id = node->getIndex(*na);

        // qDebug() << "(*na)[" << db_array[lastRead]->node_id << "]: " << parent_gist_id;
        if (parent_gist_id == 0 && alt == 2) {
            qDebug() << "adding to root: alt = " << alt;
            qDebug() << "db_array[" << lastRead << "]: alt = " << db_array[lastRead]->alt;
        }
             


        // qDebug() << "lastRead: " << lastRead; 
        // qDebug() << "parent gist id: " << parent_gist_id;
        // qDebug() << "parent id: " << pid;

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
    
    id = data->sid;
    parent = data->parent;
    alt = data->alt;
    kids = data->kids;
    status = data->status;


    /// should probably combine this with the one in readInstance(...)
    if (Data::self->lastRead == -1) {
        Data::self->firstIndex = id;
        // qDebug() << "setting firstIndex to: " << id;
    }

    // Data::self->
    Data::self->pushInstance(id - Data::self->firstIndex,
        new DbEntry(parent - Data::self->firstIndex, alt, kids, status));
    Data::self->counter++;

    Data::self->readInstance(Data::self->_na);

    return 0;
}

void Data::pushInstance(unsigned int sid, DbEntry* entry) {
    qDebug() << "nid_to_db_id[" << sid << "] = " << counter;
    nid_to_db_id[sid] = counter;
    // qDebug() << "status: " << entry->status;
    
    // if (entry->alt > 1)
    //     qDebug() << "alt = 2, really?";
    // if (entry->alt == 0)
    //     qDebug() << "alt = 0, ok";
    // else if (entry->alt == 1)
    //     qDebug() << "alt = 1, ok";
    // qDebug() << "nid_to_db_id[" << sid << "]: " << counter; 

    db_array.push_back(entry);
}


Data::~Data(void) {
    sqlite3_close(db);

    for (vector<DbEntry*>::iterator it = db_array.begin(); it != db_array.end(); it++) {
        delete (*it);
    }
}




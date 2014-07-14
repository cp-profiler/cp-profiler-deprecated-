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
    qDebug() << "new Data";
}


void Data::show_db(void) {
    qDebug() << "***** SHOW_DB: *****";
    for (auto it = nodes_arr.begin(); it != nodes_arr.end(); it++) {
            qDebug() << "sid: " << (*it)->gid << " p: " << (*it)->parent_sid <<
            " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
    qDebug() << "***** _________________________ *****";
}

/// return false if there is nothing to read
bool Data::readInstance() {
    

    return true;

}

bool Data::isDone(void) {
    return _isDone;
}

void Data::setDone(void) {
    _isDone = true;
}

void Data::startReading(void) {
    qDebug() << "in startReading...\n";

}

int Data::handleNodeCallback(Message* msg) {
    int id, pid, alt, kids, status, restart_id;
    char thread;
    unsigned long long real_id, real_pid;

    Data& data = *Data::self;
    
    id = msg->sid;
    pid = msg->parent_sid;
    alt = msg->alt;
    kids = msg->kids;
    status = msg->status;
    thread = msg->thread;
    restart_id = msg->restart_id;

    // qDebug() << "Received node: \t" << id << " " << pid << " "
    //                 << alt << " " << kids << " " << status << " wid: "
    //                 << (int)thread << " restart: " << restart_id;    

    if (restart_id == -1)
        restart_id = 0;

    if (pid != -1) {
        real_pid = (pid | ((long long)restart_id << 32)) - data.firstIndex;
    } else 
    {
        real_pid = ~0u;
    }

    real_id = (id | ((long long)restart_id << 32)) - data.firstIndex;

    Data::self->pushInstance(real_id,
        new DbEntry(real_pid, alt, kids, thread, msg->label, status));

    // qDebug() << "Pushed node: \t" << real_id.value() - data.firstIndex << " "
    //     << real_pid.value() - data.firstIndex << " " << alt << " " << kids << " "
    //     << status << " wid: " << (int)msg->thread;

    if (pid != -1) data.lastArrived = id;

    data.readInstance();

    return 0;
}

void Data::pushInstance(unsigned long long sid, DbEntry* entry) {

    sid2aid[sid] = nodes_arr.size();

    /// is sid == nodes_arr.size? no, because there are also '-1' nodes (backjumped) that dont get counted

    nodes_arr.push_back(entry);


}

char* Data::getLabelByGid(unsigned int gid) {
    return nodes_arr[ gid2aid[gid] ]->label;
}


Data::~Data(void) {
    //sqlite3_close(db);

    for (auto it = nodes_arr.begin(); it != nodes_arr.end(); it++) {
        delete (*it);
    }
}




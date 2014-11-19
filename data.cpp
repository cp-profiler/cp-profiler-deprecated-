#include <iostream>
#include <qdebug.h>
#include <string>
#include <sstream>

#include <ctime>
#include <zmq.hpp>


#include "visualnode.hh"
#include "data.hh"

using namespace std;

Data* Data::current = 0;
int Data::instance_counter = 0;


Data::Data(TreeCanvas* tc, NodeAllocator* na, bool isRestarts)
 : _tc(tc), _na(na), _isRestarts(isRestarts) {

    _id = Data::instance_counter++;
    counter = 0;
    _isDone = false;
    qDebug() << "+++ new Data created, id: " << tc->_id;
}


void Data::show_db(void) {
    qDebug() << "***** SHOW_DB: *****";
    for (auto it = nodes_arr.begin(); it != nodes_arr.end(); it++) {
            qDebug() << "sid: " << (*it)->gid << " p: " << (*it)->parent_sid <<
            " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
    qDebug() << "***** _________________________ *****";
}

bool Data::isDone(void) {
    return _isDone;
}

bool Data::isRestarts(void) {
    return _isRestarts;
}

void Data::setDone(void) {
    QMutexLocker locker(&dataMutex);
    qDebug() << ">>> set _isDone = true";
    _isDone = true;
}

void Data::startReading(void) {
    qDebug() << "in startReading...\n";

}

int Data::handleNodeCallback(Message* msg) {

    int id, pid, alt, kids, status, restart_id;
    char thread;
    unsigned long long real_id, real_pid;

    Data& data = *Data::current;
    
    
    id = msg->sid;
    pid = msg->parent_sid;
    alt = msg->alt;
    kids = msg->kids;
    status = msg->status;
    thread = msg->thread;
    restart_id = msg->restart_id;

    // qDebug() << "Received node: \t" << id << " " << pid << " "
    //                 << alt << " " << kids << " " << status << " wid: "
    //                 << (int)thread << " restart: " << restart_id << msg->label;

    /// just so we don't have ugly numbers when not using restarts   
    if (restart_id == -1)
        restart_id = 0; 

    /// this way thread id and node id are stored in one variable
    if (pid != -1)
        real_pid = (pid | ((long long)restart_id << 32)) - data.firstIndex;
    else 
        real_pid = ~0u;

    real_id = (id | ((long long)restart_id << 32)) - data.firstIndex;

    Data::current->counter++;

    Data::current->pushInstance(real_id,
        new DbEntry(real_pid, alt, kids, thread, msg->label, status));

    if (pid != -1) data.lastArrived = id;

    return 0;
}

void Data::pushInstance(unsigned long long sid, DbEntry* entry) {
    QMutexLocker locker(&dataMutex);
    // qDebug() << "mutex lock in pushInstance";
    /// is sid == nodes_arr.size? no, because there are also '-1' nodes (backjumped) that dont get counted
    nodes_arr.push_back(entry);
    sid2aid[sid] = nodes_arr.size() - 1;

    // sid2aid[sid] = counter - 1;
    // qDebug() << "mutex unlock in pushInstance";
}

char* Data::getLabelByGid(unsigned int gid) {
    QMutexLocker locker(&dataMutex);
    // qDebug() << "mutex lock in getLabel";
    // qDebug() << "mutex unlock in getLabel";
    return nodes_arr[ gid2aid[gid] ]->label;

}


Data::~Data(void) {
    //sqlite3_close(db);

    qDebug() << "--- destruct Data";

    for (auto it = nodes_arr.begin(); it != nodes_arr.end(); it++) {
        delete (*it);
    }
}




#include <iostream>
#include <qdebug.h>
#include <string>
#include <sstream>
#include <QString>

#include <ctime>

#include <zmq.hpp>


#include "visualnode.hh"
#include "data.hh"

using namespace std;

int Data::instance_counter = 0;

ostream& operator<<(ostream& s, const DbEntry& e) {
    s << "dbEntry: {";
    s << " sid: " << e.sid;
    s << " pid: " << e.parent_sid;
    s << " alt: " << e.alt;
    s << " kids: " << e.numberOfKids;
    s << " tid: " << (int)e.thread;
    s << " }";
    return s;
}


Data::Data(TreeCanvas* tc, NodeAllocator* na, bool isRestarts)
 : _tc(tc), _id(_tc->_id), _na(na), _isRestarts(isRestarts) {

    _isDone = false;
    _prev_node_timestamp = 0;
    _time_per_node = -1; // unassigned

    begin_time = system_clock::now();
    last_interval_time = begin_time;
    last_interval_nc = 0;

    if (_tc->canvasType == CanvasType::MERGED) {
        _isDone = true;
        _total_time = 0;
    }
    
    qDebug() << "+++ new Data created, tc_id: " << _id;
}


void Data::show_db(void) {
    qDebug() << "***** SHOW_DB: *****";
    for (auto it = nodes_arr.cbegin(); it != nodes_arr.end(); it++) {
            qDebug() << "sid: " << (*it)->gid << " p: " << (*it)->parent_sid <<
            " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
    qDebug() << "***** _________________________ *****";
}


void Data::setDoneReceiving(void) {
    QMutexLocker locker(&dataMutex);

    _total_nodes = nodes_arr.size();
    _total_time = nodes_arr.back()->time_stamp;

    if (_total_time != 0) {
        _time_per_node = _total_time / _total_time;
    } else 
        qDebug() << "(!) _total_time cannot be 0";


    flush_node_rate();

    // qDebug() << "Elements in nodes_arr: " << nodes_arr.size();
    
    // qDebug() << "Size of 'nodes_arr' (bytes): " << nodes_arr.size() * sizeof(DbEntry);

    _isDone = true;
    
}

int Data::handleNodeCallback(Message* msg) {

    unsigned long long real_id, real_pid;

    current_time = system_clock::now();

    int id = msg->sid;
    int pid = msg->parent_sid;
    int alt = msg->alt;
    int kids = msg->kids;
    int status = msg->status;
    int restart_id = msg->restart_id;
    char thread = msg->thread;
    float domain = msg->domain;

    // qDebug() << "Received node: \t" << id << " " << pid << " "
    //                 << alt << " " << kids << " " << status << " wid: "
    //                 << (int)thread << " restart: " << restart_id
    //                 << "time: " << msg->time
    //                 << "label: " << msg->label;

    /// just so we don't have ugly numbers when not using restarts   
    if (restart_id == -1) restart_id = 0;

    /// this way thread id and node id are stored in one variable
    /// TODO: shouldn't I make a custom hash function instead?
    if (pid != -1)
        real_pid = (pid | ((long long)restart_id << 32));
    else 
        real_pid = ~0u;

    assert(restart_id == 0);

    real_id = (id | ((long long)restart_id << 32));

    pushInstance(real_id,
        new DbEntry(real_id,
                    real_pid,
                    alt,
                    kids,
                    thread,
                    msg->label,
                    status,
                    msg->time,
                    msg->time - _prev_node_timestamp,
                    domain));

    _prev_node_timestamp = msg->time;

    // handle node rate

    long long time_passed = static_cast<long long>(duration_cast<microseconds>(current_time - last_interval_time).count());
    
    // qDebug() << "time passed: " << time_passed;
    if (static_cast<long>(time_passed) > NODE_RATE_STEP) {
        float nr = (nodes_arr.size() - last_interval_nc) * (float)NODE_RATE_STEP / time_passed;
        node_rate.push_back(nr);
        nr_intervals.push_back(last_interval_nc);
        // qDebug() << "node rate: " << nr << " at node: " << last_interval_nc;
        last_interval_time = current_time;
        last_interval_nc = nodes_arr.size();
    }

    // system_clock::time_point after_tp = system_clock::now();
    // qDebug () << "receiving node takes: " << 
    //     duration_cast<nanoseconds>(after_tp - current_time).count() << "ns";

    return 0;
}

const char* Data::getLabelByGid(unsigned int gid) {
    QMutexLocker locker(&dataMutex);

    auto it = gid2entry.find(gid);
    if (it != gid2entry.end())
        return it->second->label;
    return "";

}


unsigned long long Data::getTotalTime(void) {

    if (_isDone)
        return _total_time;
    return nodes_arr.back()->time_stamp;
}


Data::~Data(void) {

    qDebug() << "--- destruct Data";

    for (auto it = nodes_arr.begin(); it != nodes_arr.end();) {
        delete (*it);
        it = nodes_arr.erase(it);
    }
}

/// private methods

void Data::flush_node_rate(void) {

    current_time = system_clock::now();
    long long time_passed = static_cast<long long>(duration_cast<microseconds>(current_time - last_interval_time).count());

    float nr = (nodes_arr.size() - last_interval_nc) * (float)NODE_RATE_STEP / time_passed;
    node_rate.push_back(nr);
    nr_intervals.push_back(last_interval_nc);
    nr_intervals.push_back(nodes_arr.size());

    // qDebug() << "flushed nr: " << nr << " at node: " << last_interval_nc;
}

void Data::pushInstance(unsigned long long sid, DbEntry* entry) {
    QMutexLocker locker(&dataMutex);

    /// is sid == nodes_arr.size? no, because there are also '-1' nodes (backjumped) that dont get counted
    nodes_arr.push_back(entry);

    sid2aid[sid] = nodes_arr.size() - 1;
    
    qDebug() << "sid2aid[" << sid << "] = " << sid2aid[sid];

}
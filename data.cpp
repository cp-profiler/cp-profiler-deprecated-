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
 */

#include <iostream>
#include <qdebug.h>
#include <string>
#include <sstream>
#include <QString>
#include <ctime>

#include "third-party/zmq.hpp"

#include "visualnode.hh"
#include "data.hh"
#include "message.pb.hh"

using namespace std;

int Data::instance_counter = 0;

// #define SSTR( x ) dynamic_cast< std::ostringstream & >( \
//         ( std::ostringstream() << std::dec << x ) ).str()

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
    current_time = begin_time;
    last_interval_time = begin_time;
    last_interval_nc = 0;

    if (_tc->canvasType == CanvasType::MERGED) {
        _isDone = true;
        _total_time = 0;
    }

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

    _isDone = true;

}

int Data::handleNodeCallback(message::Node& node) {

    unsigned long long real_id, real_pid;

    auto prev_node_time = current_time;
    current_time = system_clock::now();

    auto node_time = duration_cast<microseconds>(current_time - prev_node_time).count();

    if (nodes_arr.size() == 0) node_time = 0; /// ignore the first node

    int id = node.sid();
    int pid = node.pid();
    int alt = node.alt();
    int kids = node.kids();
    int status = node.status();
    int restart_id = node.restart_id();
    char thread = node.thread_id();
    float domain = node.domain_size();

    // qDebug() << "Received node: \t" << id << " " << pid << " "
    //                 << alt << " " << kids << " " << status << " wid: "
    //                 << (int)thread << " restart: " << restart_id
    //                 << "time: " << node.time()
    //                 << "label: " << node.label().c_str()
    //                 << "nogood: " << node.nogood().c_str();


    if (node.has_nogood() && node.nogood().length() > 0) {
        // qDebug() << "(!)" << id << " -> " << node.nogood().c_str();
        sid2nogood[id] = node.nogood();
    }

    if (node.has_info() && node.info().length() > 0) {
        sid2info[id] = string("sid: ") + std::to_string(id) + "\n" + node.info() + "\nnogood: " + node.nogood();
    }
    
    /// just so we don't have ugly numbers when not using restarts
    if (restart_id == -1) restart_id = 0;

    /// this way thread id and node id are stored in one variable
    /// TODO: shouldn't I make a custom hash function instead?
    if (pid != -1)
        real_pid = (pid | ((long long)restart_id << 32));
    else
        real_pid = ~0u;

    real_id = (id | ((long long)restart_id << 32));

    std::string label = node.label();


    pushInstance(real_id,
        new DbEntry(real_id,
                    real_pid,
                    alt,
                    kids,
                    thread,
                    label,
                    status,
                    node.time(),
                    node_time,
                    domain));

    _prev_node_timestamp = node.time();

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

const std::string Data::getLabel(unsigned int gid) {
    QMutexLocker locker(&dataMutex);

    auto it = gid2entry.find(gid);
    if (it != gid2entry.end())
        return it->second->label;
    return "";

}

unsigned long long Data::gid2sid(unsigned int gid) {
    QMutexLocker locker(&dataMutex);

    return gid2entry.at(gid)->sid;

}

unsigned long long Data::getTotalTime(void) {

    if (_isDone)
        return _total_time;
    return nodes_arr.back()->time_stamp;
}


Data::~Data(void) {

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

    // qDebug() << "sid2aid[" << sid << "] = " << sid2aid[sid];

}

unsigned int Data::size() {
    return nodes_arr.size();
}
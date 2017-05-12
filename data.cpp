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
#include "data.hh"
#include "namemap.hh"
#include "cpprofiler/utils/utils.hh"

#include <iostream>
#include <qdebug.h>
#include <string>
#include <sstream>
#include <QString>
#include <ctime>

#include "visualnode.hh"
#include "message.pb.hh"

using namespace std;
using namespace std::chrono;

ostream& operator<<(ostream& s, const DbEntry& e) {
    s << "dbEntry: {";
    s << " sid: "  << e.restart_id << '_' << e.s_node_id;
    s << " gid: "  << e.gid;
    s << " pid: "  << e.restart_id << '_' << (int)e.parent_sid;
    s << " alt: "  << e.alt;
    s << " kids: " << e.numberOfKids;
    s << " tid: "  << e.thread_id;
    s << " restart: " << e.restart_id;
    s << " }";
    return s;
}


Data::Data() {
    _isDone = false;
    _prev_node_timestamp = 0;
    _time_per_node = -1; // unassigned

    begin_time = system_clock::now();
    current_time = begin_time;
    last_interval_time = begin_time;
    last_interval_nc = 0;
}

void Data::setDoneReceiving(void) {

    qDebug() << "done receiving";
    QMutexLocker locker(&dataMutex);

    // _total_nodes = nodes_arr.size();
    _total_time = nodes_arr.back()->time_stamp;

    if (_total_time != 0) {
        _time_per_node = _total_time / _total_time;
    } else {
        qDebug() << "(!) _total_time cannot be 0";
    }

    /// *** Flush_node_rate ***

    current_time = system_clock::now();
    long long time_passed = static_cast<long long>(
        duration_cast<microseconds>(current_time - last_interval_time).count());

    float nr = (nodes_arr.size() - last_interval_nc) * (float)NODE_RATE_STEP / time_passed;
    node_rate.push_back(nr);
    nr_intervals.push_back(last_interval_nc);
    nr_intervals.push_back(nodes_arr.size());

    _isDone = true;

}


int Data::handleNodeCallback(message::Node& node) {

    auto prev_node_time = current_time;
    current_time = system_clock::now();

    auto node_time = duration_cast<microseconds>(current_time - prev_node_time).count();

    if (nodes_arr.size() == 0) node_time = 0; /// ignore the first node

    int sid = node.sid();
    int pid = node.pid();
    int alt = node.alt();
    int kids = node.kids();
    int status = node.status();
    int restart_id = node.restart_id();
    int tid = node.thread_id();
    float domain = node.domain_size();
    int nogood_bld = node.nogood_bld();
    bool usesAssumptions = node.uses_assumptions();
    int backjump_distance = node.backjump_distance();
    int decision_level = node.decision_level();

    /// thread id and node id are stored in one variable (for hashing)
    int64_t real_pid = -1;
    if (pid != -1) {
        real_pid = (pid | ((int64_t)restart_id << 32));
    }

    const std::string& label = node.label();


    auto entry = new DbEntry(sid,
                    restart_id,
                    real_pid,
                    alt,
                    kids,
                    label,
                    tid,
                    status,
                    node.time(),
                    node_time,
                    domain,
                    nogood_bld,
                    usesAssumptions,
                    backjump_distance,
                    decision_level);

    /// TODO(maxim): do sid2info and sid2nogood need to be protected by a mutex?

    if (node.has_info() && node.info().length() > 0) {
        sid2info[entry->full_sid] = new std::string(node.info());
    }

    pushInstance(entry);

    if (node.has_nogood() && node.nogood().length() > 0) {

        /// simplify nogood here
        auto ng = node.nogood();

        if (nameMap) {
            auto q_ng = QString::fromStdString(ng);
            ng = nameMap->replaceNames(q_ng, true).toStdString();
        }

        sid2nogood[entry->full_sid] = ng;
    }

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

std::string Data::getLabel(int gid) {
    QMutexLocker locker(&dataMutex);

    auto it = gid2entry.find(gid);
    if (it != gid2entry.end() && it->second != nullptr)
        return it->second->label;
    return "";

}

int64_t Data::gid2sid(int gid) const {
    QMutexLocker locker(&dataMutex);

    /// not for any gid there is entry (TODO: there should be a 'default' one)
    auto it = gid2entry.find(gid);
    if (it != gid2entry.end())
        return gid2entry.at(gid)->full_sid;
    return -1;

}

unsigned long long Data::getTotalTime(void) {

    if (nodes_arr.size() == 0) return 0;

    if (_isDone)
        return _total_time;
    return nodes_arr.back()->time_stamp;
}


Data::~Data(void) {

    for (auto it = nodes_arr.begin(); it != nodes_arr.end();) {
        delete (*it);
        it = nodes_arr.erase(it);
    }

    for (auto it = sid2info.begin(); it != sid2info.end();) {
        delete it->second;
        it = sid2info.erase(it);
    }
}

/// ***********************
/// *** private methods ***
/// ***********************

// NOTE(maxim): this can be replaced with multiple arrays: one for each restart 
void Data::pushInstance(DbEntry* entry) {
    QMutexLocker locker(&dataMutex);

    /// NOTE(maxim): `sid` != `nodes_arr.size`, because there are also
    /// '-1' nodes (backjumped) that dont get counted

    nodes_arr.push_back(entry);

    auto full_sid = entry->full_sid;
    sid2aid[full_sid] = nodes_arr.size() - 1;
}

void Data::setNameMap(NameMap* names) {
    nameMap = names;
}

#ifdef MAXIM_DEBUG

/// NOTE(maxim): creates new entry if does not exist yet
void Data::setLabel(int gid, const std::string& str) {
    QMutexLocker locker(&dataMutex);

    auto it = gid2entry.find(gid);
    if (it != gid2entry.end() && it->second != nullptr) {
        it->second->label = str;
    } else {

        static int dummy_sid = 0;
        dummy_sid++;

        auto entry = new DbEntry{};
        nodes_arr.push_back(entry);
        gid2entry[gid] = entry;
        sid2aid[dummy_sid] = nodes_arr.size() - 1;

        entry->label = str;
    }
}

const std::string Data::getDebugInfo() const {
    std::ostringstream os;

    os << "---nodes_arr---" << '\n';
    for (auto it = nodes_arr.cbegin(); it != nodes_arr.end(); ++it) {
      os << **(it) << "\n";
    }
    os << "---------------" << '\n';

    os << "---sid2nogood---" << '\n';
    for (auto it = sid2nogood.cbegin(); it != sid2nogood.end();
         it++) {
      os << it->first << " -> " << it->second << "\n";
    }
    os << "---------------" << '\n';

    return os.str();
}

#endif


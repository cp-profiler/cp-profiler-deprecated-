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
#include "cpprofiler/utils/literals.hh"

#include <iostream>
#include <qdebug.h>
#include <string>
#include <sstream>
#include <QString>
#include <ctime>

#include "visualnode.hh"

#include "message.hh"

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

using TP = system_clock::time_point;

static uint64_t milliseconds_passed(TP begin, TP end) {
  return static_cast<uint64_t>(
      duration_cast<microseconds>(end - begin).count());
}

class NodeTimer {
  /// step for node rate counter (in microseconds)
  static constexpr int NODE_RATE_STEP = 1000;

  using system_clock = std::chrono::system_clock;

  TP begin_time;
  TP last_interval_time;
  TP current_time;

  // Total solver time in microseconds
  uint64_t m_total_time;

  bool finished{false};

 public:
  void start() {
    begin_time = system_clock::now();
    current_time = begin_time;
  }

  void end() {
    m_total_time = milliseconds_passed(begin_time, current_time);
    finished = true;
  }

  void on_node() {
    current_time = system_clock::now();

    auto time_passed = milliseconds_passed(last_interval_time, current_time);

    // if (static_cast<long>(time_passed) > NODE_RATE_STEP) {
    //     float nr = (nodes_arr.size() - last_interval_nc) * (float)NODE_RATE_STEP / time_passed;
    //     node_rate.push_back(nr);
    //     nr_intervals.push_back(last_interval_nc);
    //     qDebug() << "node rate: " << nr << " at node: " << last_interval_nc;
    //     last_interval_time = current_time;
    //     last_interval_nc = nodes_arr.size();
    // }

  }

  uint64_t total_time() { return finished ? m_total_time : 0; }
};

Data::Data() : search_timer{new NodeTimer} {

  // last_interval_time = begin_time;
  // last_interval_nc = 0;
}

void Data::setDoneReceiving(void) {
    QMutexLocker locker(&dataMutex);

    search_timer->end();
    qDebug() << "done receiving";

    /// *** Flush_node_rate ***

    // current_time = system_clock::now();
    // auto time_passed = static_cast<uint64_t>(
    //     duration_cast<microseconds>(current_time - last_interval_time).count());

    // float nr = (nodes_arr.size() - last_interval_nc) * (float)NODE_RATE_STEP / time_passed;
    // node_rate.push_back(nr);
    // nr_intervals.push_back(last_interval_nc);
    // nr_intervals.push_back(nodes_arr.size());

    _isDone = true;

}




int Data::handleNodeCallback(const cpprofiler::Message& node) {

    search_timer->on_node();

    int sid = node.id();
    int pid = node.pid();
    int alt = node.alt();
    int kids = node.kids();
    int status = node.status();

    int restart_id = node.restart_id();
    // int restart_id = -1;
    int tid = node.thread_id();

    int64_t real_pid = -1;
    if (pid != -1) {
        real_pid = (pid | ((int64_t)restart_id << 32));
    }

    auto entry = new DbEntry(sid, real_pid, alt, kids, status);

    // if (node.has_restart_id()) {
        entry->restart_id = node.restart_id();
    // }

    entry->label = node.has_label() ? node.label() : "";

    if (node.has_info() && node.info().length() > 0) {
        sid2info[entry->full_sid] = new std::string(node.info());
    }

    if (node.has_thread_id()) {
        entry->thread_id = node.thread_id();
    }

    pushInstance(entry);

    if (node.has_nogood() && node.nogood().length() > 0) {

        /// simplify nogood here
        NogoodViews ng(node.nogood());

        if (nameMap) {
            string renamed = nameMap->replaceNames(ng.original, true);
            ng.renamed = utils::lits::remove_redundant_wspaces(renamed);
            ng.simplified = utils::lits::simplify_ng(ng.renamed);
        }

#ifdef MAXIM_DEBUG
        qDebug() << "add nogood: " << entry->full_sid << " " << ng.original.c_str();
#endif

        sid2nogood[entry->full_sid] = ng;
    }

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

uint64_t Data::getTotalTime() {
    return search_timer->total_time();
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
      os << it->first << " -> " << it->second.original << "\n";
    }
    os << "---------------" << '\n';

    return os.str();
}

#endif


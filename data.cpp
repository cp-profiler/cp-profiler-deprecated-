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

#include "third-party/json.hpp"

#include <iostream>
#include <qdebug.h>
#include <string>
#include <sstream>
#include <QString>
#include <ctime>

#include "visualnode.hh"

using namespace std;
using namespace std::chrono;

#include "submodules/cpp-integration/message.hpp"

ostream& operator<<(ostream& s, const NodeUID& uid) {
    s << "{" << uid.nid << ", " << uid.tid << ", " << uid.rid << "}";
    return s;
}

ostream& operator<<(ostream& s, const DbEntry& e) {
    s << "dbEntry: {";
    s << " uid: "   << e.nodeUID;
    s << " p_uid: " << e.parentUID.nid;
    s << " gid: "   << e.gid;
    s << " alt: "   << e.alt;
    s << " kids: "  << e.numberOfKids;
    s << " tid: "   << e.thread_id;
    s << " restart: "  << e.nodeUID.rid;
    s << " }";
    return s;
}

using TP = system_clock::time_point;

static uint64_t microseconds_passed(TP begin, TP end) {
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

  bool started{false};
  bool finished{false};

 public:
  void start() {
    if (started) {
        qDebug() << "timer already started";
        return;
    }
    started = true;
    begin_time = system_clock::now();
    last_interval_time = begin_time;
    current_time = begin_time;
  }

  void end() {
    if (!started) {
        qDebug() << "timer not started";
        return;
    }
    m_total_time = microseconds_passed(begin_time, current_time);
    finished = true;
  }

  uint64_t on_node() {
    current_time = system_clock::now();

    uint64_t time_passed = microseconds_passed(last_interval_time, current_time);

    last_interval_time = current_time;

    return time_passed;
  }

  uint64_t total_time() { return finished ? m_total_time : 0; }
};

Data::Data() : search_timer{new NodeTimer}, nameMap{nullptr} {}

void Data::initReceiving() {
    QMutexLocker locker(&dataMutex);

    search_timer->start();
}

void Data::setDoneReceiving() {
    QMutexLocker locker(&dataMutex);

    search_timer->end();

    _isDone = true;
}

void Data::handleNodeCallback(const cpprofiler::Message& node) {
    QMutexLocker locker(&dataMutex);
    uint64_t node_time = search_timer->on_node();

    auto n_uid = node.nodeUID();
    auto p_uid = node.parentUID();

    NodeUID nodeUID {n_uid.nid, n_uid.rid, n_uid.tid};
    NodeUID parentUID {p_uid.nid, p_uid.rid, p_uid.tid};

    auto entry = new DbEntry(nodeUID, parentUID, node.alt(), node.kids(), node.status());

    entry->node_time = node_time;

    entry->label = node.has_label() ? node.label() : "";

    if (node.has_info() && node.info().length() > 0) {
        uid2info[nodeUID] = make_shared<std::string>(node.info());

        try {
            auto info_json = nlohmann::json::parse(node.info());
            auto obj_value = info_json.find("objective");

            if(obj_value != info_json.end()) {
                auto el = (*obj_value)[0];
                if (el.is_number()) {
                    uid2obj[nodeUID] = el.get<int>();
                }
            }
        } catch (std::exception& e) {
            std::cerr << "Can't parse json near objective: " << e.what() << "\n";
        }

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

        uid2nogood[entry->nodeUID] = ng;
    }
}

std::string Data::getLabel(int gid) {
    QMutexLocker locker(&dataMutex);

    auto it = gid2entry.find(gid);
    if (it != gid2entry.end() && it->second != nullptr)
        return it->second->label;
    return "";

}

NodeUID Data::gid2uid(int gid) const {
    QMutexLocker locker(&dataMutex);

    /// not for any gid there is entry (TODO: there should be a 'default' one)
    auto it = gid2entry.find(gid);
    if (it != gid2entry.end())
        return gid2entry.at(gid)->nodeUID;
    return {-1, -1, -1};

}

uint64_t Data::getTotalTime() {
    QMutexLocker locker(&dataMutex);
    return search_timer->total_time();
}


Data::~Data(void) {
    QMutexLocker locker(&dataMutex);
    for (auto it = nodes_arr.begin(); it != nodes_arr.end();) {
        delete (*it);
        it = nodes_arr.erase(it);
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
    uid2aid[entry->nodeUID] = nodes_arr.size() - 1;
}

void Data::setNameMap(NameMap* names) {
    QMutexLocker locker(&dataMutex);
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

        auto entry = new DbEntry{};
        nodes_arr.push_back(entry);
        gid2entry[gid] = entry;

        static int dummy_sid = 0;
        dummy_sid++;
        NodeUID dummy_uid{dummy_sid, -1, -1};

        uid2aid[dummy_uid] = nodes_arr.size() - 1;

        entry->label = str;
    }
}

const std::string Data::getDebugInfo() const {
    QMutexLocker locker(&dataMutex);
    std::ostringstream os;

    os << "---nodes_arr---" << '\n';
    for (auto it = nodes_arr.cbegin(); it != nodes_arr.end(); ++it) {
      os << **(it) << "\n";
    }
    os << "---------------" << '\n';

    os << "---sid2nogood---" << '\n';
    for (auto it = uid2nogood.cbegin(); it != uid2nogood.end();
         it++) {
      os << it->first << " -> " << it->second.original << "\n";
    }
    os << "---------------" << '\n';

    return os.str();
}

#endif


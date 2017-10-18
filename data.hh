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
 *
 */

#ifndef DATA_HH
#define DATA_HH

#include <vector>
#include <unordered_map>
#include <QTimer>
#include <chrono>
#include <QMutex>

#include <iostream>
#include <string>
#include <memory>

#include <QDebug>

#include <cstdint>
#include <cassert>

#include "nogood_representation.hh"
#include "cpprofiler/universal.hh"

class NameMap;
namespace cpprofiler {
class Message;
}

class DbEntry {

public:
    DbEntry(NodeUID uid, NodeUID parent_uid, int _alt, int _kids,
            std::string _label, int tid, int _status, int64_t _time_stamp,
            int64_t _node_time) :
        nodeUID(uid), parentUID(parent_uid), alt(_alt), numberOfKids(_kids), label(_label), thread_id(tid), time_stamp(_time_stamp), node_time(_node_time), status(_status)
    {
        /// Map message status to profiler status;
        /// alternatively could make treebuilder use message status
        // switch (_status) {
        //     case 0:

        //     break;
        // }
        // status 
    }

    DbEntry(NodeUID uid, NodeUID parent_uid, int alt, int kids, int status)
        : nodeUID(uid),
          parentUID(parent_uid),
          alt(alt),
          numberOfKids(kids),
          status(status) {}

    DbEntry() = default;

    friend std::ostream& operator<<(std::ostream& s, const DbEntry& e);

    NodeUID nodeUID;
    NodeUID parentUID;

    int32_t gid {-1}; // gist id, set to -1 so we don't forget to assign the real value
    int32_t alt; // which child by order
    int32_t numberOfKids;
    std::string label;
    int32_t thread_id{-1};
    int32_t depth {-1};
    uint64_t time_stamp{0};
    uint64_t node_time{0};
    char status;
};

class NodeTimer;

class Data : public QObject {
Q_OBJECT

    std::unique_ptr<NodeTimer> search_timer;

    std::vector<DbEntry*> nodes_arr;

    // Whether received DONE_SENDING message
    bool _isDone{false};

    /// How many nodes received within each NODE_RATE_STEP interval
    std::vector<float> node_rate;

    int last_interval_nc;

    /// Map solver Id to no-good string (rid = 0 always for chuffed)
    Uid2Nogood uid2nogood;

    NameMap* nameMap;

    /// node rate intervals
    std::vector<int> nr_intervals;

    std::unordered_map<NodeUID, int> uid2obj;

public:

    /// Mapping from solver Id to array Id (nodes_arr)
    /// can't use vector because sid is too big with threads
    std::unordered_map<NodeUID, int> uid2aid;

    /// Maps gist Id to dbEntry (possibly in the other Data instance);
    /// i.e. needed for a merged tree to show labels etc.
    /// TODO(maixm): this should probably be a vector?
    std::unordered_map<int, DbEntry*> gid2entry;

    std::unordered_map<NodeUID, std::shared_ptr<std::string>> uid2info;

    /// synchronise access to data entries
    mutable QMutex dataMutex {QMutex::Recursive};

private:

    /// Populate nodes_arr with the data coming from
    void pushInstance(DbEntry* entry);

public:

    Data();
    ~Data();

    void handleNodeCallback(const cpprofiler::Message& node);

    /// TODO(maxim): Do I want a reference here?
    /// return label by gid (Gist ID)
    std::string getLabel(int gid);

    /// return solver id by gid (Gist ID)
    NodeUID gid2uid(int gid) const;

    void connectNodeToEntry(int gid, DbEntry* const entry);

    /// return total number of nodes
    int size() const { return nodes_arr.size(); }

/// ********* GETTERS **********

    bool isDone(void) const { return _isDone; }

    const std::vector<DbEntry*>& getEntries() const { return nodes_arr; }
    inline const Uid2Nogood& getNogoods(void) { return uid2nogood; }

    uint64_t getTotalTime();

    int32_t getGidByUID(NodeUID uid) {
        return nodes_arr[uid2aid[uid]]->gid;
    }

    const int* getObjective(NodeUID uid) const {
        auto it = uid2obj.find(uid);
        if (it != uid2obj.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }
    /// NOTE(maxim): this only works for a merged tree now?
    DbEntry* getEntry(int gid) const;

    const NameMap* getNameMap() const { return nameMap; }
    void setNameMap(NameMap* names);

    const std::vector<int>& node_rate_intervals() const { return nr_intervals; }


/// ****************************

/// Starts node timer
void initReceiving();

    public Q_SLOTS:

    void setDoneReceiving();

#ifdef MAXIM_DEBUG
    void setLabel(int gid, const std::string& str);
    const std::string getDebugInfo() const;
#endif
};

inline
void Data::connectNodeToEntry(int gid, DbEntry* const entry) {
    gid2entry[gid] = entry;
}

inline
DbEntry* Data::getEntry(int gid) const {

    auto it = gid2entry.find(gid);
    if (it != gid2entry.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}


#endif // DATA_HH


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

#include <QDebug>


#include "node.hh"
#include "visualnode.hh"

typedef NodeAllocatorBase<VisualNode> NodeAllocator;

using namespace std::chrono;
using std::string;
using std::ostream;

namespace message {
    class Node;
}

enum MsgType {
  NODE_DATA = 1,
  DONE_SENDING = 2,
  START_SENDING = 3
};

class DbEntry {
private:

public:
    DbEntry(unsigned long long id, unsigned long long _p, int _alt, int _kids, char _tid,
            std::string _label, int _status, unsigned long long _time_stamp,
            unsigned long long _node_time, float _domain) :
        sid(id), gid(-1), parent_sid(_p), alt(_alt), numberOfKids(_kids),
        status(_status), label(_label), thread(_tid), depth(-1), time_stamp(_time_stamp), node_time(_node_time),
        domain(_domain) {
    }

    friend ostream& operator<<(ostream& s, const DbEntry& e);

    int sid; // solver id
    int gid; // gist id, set to -1 so we don't forget to assign the real value
    unsigned long long parent_sid; // parent id in database
    int alt; // which child by order
    int numberOfKids;
    int status;
    std::string label;
    char thread;
    int depth;
    int decisionLevel;
    unsigned long long time_stamp;
    unsigned long long node_time;
    float domain;
};

class Data : public QObject {
Q_OBJECT

// friend class TreeBuilder;
// friend class PixelTreeCanvas;

/// step for node rate counter (in microseconds)
static const int NODE_RATE_STEP = 1000;

public:
    /// counts instances of Data
    static int instance_counter;

    // const TreeCanvas* _tc; /// TreeCanvas instance it belongs to
    // const int _id; /// Id of the TreeCanvas instance it belongs to

    /// TODO: do I even need NodeAllocator in Data?
    const NodeAllocator* _na; /// Node allocator of _tc

    /// True if we want a dummy node (needed for showing restarts)
    bool _isRestarts = false;

    /// Where most node data is stored, id as it comes from Broker
    std::vector<DbEntry*> nodes_arr;

    /// Mapping from solver Id to array Id (nodes_arr)
    /// can't use vector because sid is too big with threads
    std::unordered_map<unsigned long long, int> sid2aid;

    /// Mapping from gist Id to array Id (nodes_arr)
    // std::vector<unsigned long long> gid2aid; /// use gid2entry instead

    /// Maps gist Id to dbEntry (possibly in the other Data instance);
    /// i.e. needed for a merged tree to show labels etc.
    /// TODO(maixm): this should probably be a vector?
    std::unordered_map<unsigned int, DbEntry*> gid2entry;

    /// Map solver Id to no-good string
    std::unordered_map<unsigned long long, string> sid2nogood;

    std::unordered_map<unsigned long long, string*> sid2info;

    // Whether received DONE_SENDING message
    bool _isDone;

    // Name of the FlatZinc model
    string _title;

    // Total solver time in microseconds
    unsigned long long _total_time;

    unsigned long int _prev_node_timestamp;
    int _total_nodes;

    /// How many nodes received within each NODE_RATE_STEP interval
    std::vector<float> node_rate;

    /// On which node each interval starts
    std::vector<int> nr_intervals;

    /// derived properties
    int _time_per_node;

    /// for node rate
    system_clock::time_point begin_time;
    system_clock::time_point last_interval_time;
    system_clock::time_point current_time;

    int last_interval_nc;

public:

    /// used to access Data instance from different threads (in parallel solver)
    QMutex dataMutex;

private:

    /// Populate nodes_arr with the data coming from
    void pushInstance(unsigned long long sid, DbEntry* entry);

    /// Work out node rate for the last (incomplete) interval
    void flush_node_rate(void);

public:

    Data(NodeAllocator* na);
    ~Data(void);

    int handleNodeCallback(message::Node& node);

    void show_db(void); /// TODO: write to a file

    /// TODO(maxim): Do I want a reference here?
    /// return label by gid (Gist ID)
    std::string getLabel(unsigned int gid);

    /// return solver id by gid (Gist ID)
    unsigned long long gid2sid(unsigned int gid);

    void connectNodeToEntry(unsigned int gid, DbEntry* const entry);

    /// return total number of nodes
    unsigned int size();

/// ********* GETTERS **********

    // int id(void) { return _id; }

    bool isDone(void) { return _isDone; }
    bool isRestarts(void) { return _isRestarts; }
    string getTitle(void) { return _title; }
    inline const std::unordered_map<unsigned long long, string>& getNogoods(void) { return sid2nogood; }
    inline std::unordered_map<unsigned long long, string*>& getInfo(void) { return sid2info; }

    unsigned long long getTotalTime(void); /// time in microseconds

    DbEntry* getEntry(unsigned int gid) const;
    DbEntry* getEntry(const Node& node) const;

    const string* getNogood(const Node& node) const;
    const string* getInfo(const Node& node) const;

    unsigned int getGidBySid(unsigned int sid) { return nodes_arr[sid2aid[sid]]->gid; }


/// ****************************

/// ********* SETTERS **********

    void setTitle(string title) { _title = title; }
    void setIsRestarts(bool isRestarts) {
        /// can't set true to false
        assert(_isRestarts == false || isRestarts == true);
        _isRestarts = isRestarts;
    }


/// ****************************

    public Q_SLOTS:

    // sets _isDone to true when received DONE_SENDING
    void setDoneReceiving(void);
};

inline
void Data::connectNodeToEntry(unsigned int gid, DbEntry* entry) {
    gid2entry[gid] = entry;
}

inline
DbEntry* Data::getEntry(unsigned int gid) const {
    std::unordered_map<unsigned int, DbEntry*>::const_iterator it = gid2entry.find(gid);
    if (it != gid2entry.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}


#endif // DATA_HH


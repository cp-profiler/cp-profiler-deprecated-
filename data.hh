#ifndef DATA_HH
#define DATA_HH

#include <vector>
#include <unordered_map>
#include <QTimer>
#include <chrono>
#include <QMutex>

#include <iostream>
#include <string>

#include "treebuilder.hh"
#include "treecanvas.hh"
#include "node.hh"

typedef NodeAllocatorBase<VisualNode> NodeAllocator;

using namespace std::chrono;
using std::string;
using std::ostream;

enum MsgType {
  NODE_DATA = 1,
  DONE_SENDING = 2,
  START_SENDING = 3
};

struct Message {
    static const int LABEL_SIZE = 32;

    MsgType type;
    int sid;
    int parent_sid;
    int alt;
    int kids;
    int status;
    int restart_id;
    unsigned long long time;
    char thread;
    char label[LABEL_SIZE];
    float domain;
};

class DbEntry {
private:

public:
    DbEntry(unsigned long long id, unsigned long long _p, int _alt, int _kids, char _tid,
            char* _label, int _status, unsigned long long _time_stamp,
            unsigned long long _node_time, float _domain) :
        sid(id), gid(-1), parent_sid(_p), alt(_alt), numberOfKids(_kids),
        status(_status), thread(_tid), depth(-1), time_stamp(_time_stamp), node_time(_node_time),
        domain(_domain) {
          
          memcpy(label, _label, Message::LABEL_SIZE);
    }

    friend ostream& operator<<(ostream& s, const DbEntry& e);

    DbEntry(): gid(-1) {}

    int sid; // TODO: redundant, only for debugging
    int gid; // gist id, set to -1 so we don't forget to assign the real value
    unsigned long long parent_sid; // parent id in database 
    int alt; // which child by order
    int numberOfKids;
    int status;
    char label[Message::LABEL_SIZE];
    char thread;
    int depth;
    unsigned long long time_stamp;
    unsigned long long node_time;
    float domain;
};

class Data : public QObject {
Q_OBJECT

friend class TreeBuilder;
friend class PixelTreeCanvas;

/// step for node rate counter (in microseconds)
static const int NODE_RATE_STEP = 1000;

private:
    /// counts instances of Data
    static int instance_counter;

    const TreeCanvas* _tc; /// TreeCanvas instance it belongs to
    const int _id; /// Id of the TreeCanvas instance it belongs to
    const NodeAllocator* _na; /// Node allocator of _tc

    /// True if we want a dummy node (needed for showing restarts)
    const bool _isRestarts;

    /// Where most node data is stored, id as it comes from Broker
    std::vector<DbEntry*> nodes_arr;

    /// Mapping from solver Id to array Id (nodes_arr)
    /// can't use vector because sid is too big with threads
    std::unordered_map<unsigned long long, int> sid2aid;

    /// Mapping from gist Id to array Id (nodes_arr)
    // std::vector<unsigned long long> gid2aid; /// use gid2entry instead

    /// Maps gist Id to dbEntry (possibly in the other Data instance);
    /// i.e. needed for a merged tree to show labels etc.
    std::unordered_map<unsigned int, DbEntry*> gid2entry;

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

    Data(TreeCanvas* tc, NodeAllocator* na, bool isRestarts);
    ~Data(void);

    int handleNodeCallback(Message* data);

    void show_db(void); /// TODO: write to a file

    /// return label by gid (Gist ID)
    const char* getLabel(unsigned int gid);

    void connectNodeToEntry(unsigned int gid, DbEntry* const entry);

/// ********* GETTERS **********

    int id(void) { return _id; }

    bool isDone(void) { return _isDone; }
    bool isRestarts(void) { return _isRestarts; }
    string getTitle(void) { return _title; }

    unsigned long long getTotalTime(void); /// time in microseconds

    DbEntry* getEntry(unsigned int gid);


/// ****************************

/// ********* SETTERS **********

    void setTitle(char* title) { _title = title; qDebug() << "sent: " << _title.c_str();}

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
DbEntry* Data::getEntry(unsigned int gid) {
    return gid2entry[gid];
}




#endif // DATA_HH


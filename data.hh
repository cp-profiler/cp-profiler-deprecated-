#ifndef DATA_HH
#define DATA_HH

#include <vector>
#include <unordered_map>
#include <QTimer>
#include <QMutex>

#include <iostream>

#include "treebuilder.hh"
#include "treecanvas.hh"
#include "node.hh"

typedef NodeAllocatorBase<VisualNode> NodeAllocator;

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
};

class DbEntry {
private:

public:
    DbEntry(unsigned long long _p, int _alt, int _kids,
            char _tid, char* _label, int _status,
            unsigned long long _time_stamp, unsigned long long _node_time) :
        gid(-1), depth(-1), parent_sid(_p), alt(_alt), numberOfKids(_kids),
        status(_status), thread(_tid), time_stamp(_time_stamp), node_time(_node_time) {
          
          memcpy(label, _label, Message::LABEL_SIZE);
    }

    DbEntry(): gid(-1) {}

    int gid; // gist id, set to -1 so we don't forget to assign the real value
    unsigned long long parent_sid; // parent id in database 
    int alt; // which child by order
    int numberOfKids;
    int status;
    char label[Message::LABEL_SIZE];
    char thread;
    char depth;
    unsigned long long time_stamp;
    unsigned long long node_time;
};


class Data : public QObject {
Q_OBJECT

friend class TreeBuilder;

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
    std::unordered_map<unsigned long long, int> sid2aid;

    /// Mapping from gist Id to array Id (nodes_arr)
    std::unordered_map<unsigned long long, int> gid2aid;

    /// Maps gist Id to dbEntry (possibly in the other Data instance);
    /// i.e. needed for a merged tree to show labels etc.
    std::unordered_map<unsigned int, DbEntry*> gid2entry;

    // Whether received DONE_SENDING message
    bool _isDone;

    // Total solver time in microseconds
    unsigned long int _total_time;

    unsigned long int _last_node_timestamp;
    int _total_nodes;


    /// derived properties
    int _time_per_node;

public:

    /// used to access Data instance from different threads (in parallel solover)
    QMutex dataMutex;

private:

    /// Populate nodes_arr with the data coming from 
    void pushInstance(unsigned long long sid, DbEntry* entry);
    
public:

    Data(TreeCanvas* tc, NodeAllocator* na, bool isRestarts);
    ~Data(void);

    int handleNodeCallback(Message* data);

    void show_db(void); /// TODO: write to a file

    char* getLabelByGid(unsigned int gid);

    /// get label omitting gid2aid mapping (i.e. for merged tree)   /// TODO: delete if not used
    char* getLabelByAid(unsigned int aid);

    /// set label (i.e. for merged tree) 
    void setLabel(unsigned int aid, char* label);   /// TODO: delete if not used

    void connectNodeToEntry(unsigned int gid, DbEntry* entry);

/// ********* GETTERS **********

    int id(void) { return _id; }

    bool isDone(void) { return _isDone; }
    bool isRestarts(void) { return _isRestarts; }

    DbEntry* getEntry(unsigned int gid);


/// ****************************

    public Q_SLOTS:
    void startReading(void);

    // sets _isDone to true when received DONE_SENDING
    void setDoneReceiving(void);
    
};




#endif // DATA_HH


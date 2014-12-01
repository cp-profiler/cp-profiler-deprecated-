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
    static const int LABEL_SIZE = 16;

    MsgType type;
    int sid;
    int parent_sid;
    int alt;
    int kids;
    int status;
    int restart_id;
    char thread;
    char label[LABEL_SIZE];
};

class DbEntry {
private:

public:
    DbEntry(unsigned long long _p, int _alt, int _kids, char _tid, char* _label, int _status) :
        gid(-1), parent_sid(_p), alt(_alt), numberOfKids(_kids),
        status(_status), thread(_tid) {
          
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

    /// where most node data is stored, id as it comes from Broker
    std::vector<DbEntry*> nodes_arr;

public:

    /// used to access Data instance from different threads (in parallel solover)
    QMutex dataMutex;

private:
    
    int totalElements = -1;


    /// **** for restarts ****
    std::vector<int> restarts_offsets;

    /// isRestarts true if we want a dummy node (needed for showing restarts)
    bool _isRestarts;

    bool _isDone;

    /// mapping from solver Id to array Id (nodes_arr)
    std::unordered_map<unsigned long long, int> sid2aid;

    /// mapping from gist Id to array Id (nodes_arr)
    std::unordered_map<unsigned long long, int> gid2aid;

public Q_SLOTS:
    void startReading(void);
    
public:

    Data(TreeCanvas* tc, NodeAllocator* na, bool isRestarts);
    ~Data(void);

    static Data* current;

    void pushInstance(unsigned long long sid, DbEntry* entry);

    // whether received DONE_SENDING message
    bool isDone(void);

    bool isRestarts(void);

    // sets _isDone to true when received DONE_SENDING
    void setDone(void);
    
    char* getLabelByGid(unsigned int gid);

    /// get label omitting gid2aid mapping (i.e. for merged tree)
    char* getLabelByAid(unsigned int aid);

    /// set label (i.e. for merged tree) 
    void setLabel(unsigned int aid, char* label);

    void show_db(void);

    static int handleNodeCallback(Message* data);

/// ********* GETTERS **********
    int id(void) { return _id; }
    
};




#endif // DATA_HH


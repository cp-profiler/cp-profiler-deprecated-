#ifndef DATA_HH
#define DATA_HH

#include <vector>
#include <unordered_map>
#include <sqlite3.h>
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
    char label[16];
};

class DbEntry {
private:

public:
    DbEntry(unsigned long long _p, int _alt, int _kids, char _tid, char* _label, int _status) :
        gid(-1), parent_sid(_p), alt(_alt), numberOfKids(_kids),
        status(_status), thread(_tid) {
          
          memcpy(label, _label, Message::LABEL_SIZE);
          // std::cout << label << std::endl;
    }

    DbEntry(): gid(-1) {}

    int gid; // gist Id
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

public:

    /// id of the Data instance
    int _id;
    
    int counter;
    QMutex dataMutex;

    static const int READING_PERIOD = 1000;

    void show_db(void);

private:

    static const int PORTION = 50000;

    
    /// counts instances of Data
    static int instance_counter;

    long long lastRead;
    int firstIndex = 0; // for nodes_arr
    int restarts_offset = 0; // index of the last node from previous restart
    int lastArrived = 0; // ???

    int nextToRead = 0;
    int totalElements = -1;

    sqlite3 *db;
    TreeCanvas* _tc;
    NodeAllocator* _na;
    std::vector<DbEntry*> nodes_arr;

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
    static int handleNodeCallback(Message* data);
    
};




#endif // DATA_HH


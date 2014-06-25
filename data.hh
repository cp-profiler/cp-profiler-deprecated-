#ifndef DATA_HH
#define DATA_HH

#include <vector>
#include <tr1/unordered_map>
#include <sqlite3.h>
#include <QTimer>

#include <iostream>

#include "treecanvas.hh"
#include "node.hh"

typedef NodeAllocatorBase<VisualNode> NodeAllocator;

enum MsgType {
  NODE_DATA = 1,
  DONE_SENDING = 2,
  START_SENDING = 3
};

class BigId {

private:
    unsigned long long _id;
public:
    BigId(unsigned long long id) : _id(id) {

    };

    void operator=(unsigned long long id) {
        _id = id;
    }

    unsigned long long value(void) {
        return _id;
    }
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
        thread(_tid), status(_status) {
          
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
public:
    static const int READING_PERIOD = 1000;

private:

    static const int PORTION = 50000;

    int lastRead;
    int firstIndex = 0; // for nodes_arr
    int restarts_offset = 0; // index of the last node from previous restart
    int lastArrived = 0;

    /// isRestarts true if we want a dummy node (needed for showing restarts)
    bool _isRestarts;

    int nextToRead = 0;
    int totalElements = -1;

    sqlite3 *db;
    TreeCanvas* _tc;
    NodeAllocator* _na;
    std::vector<DbEntry*> nodes_arr;

    /// **** for restarts ****
    std::vector<int> restarts_offsets;


    /// mapping from solver Id to array Id (nodes_arr)
    std::tr1::unordered_map<unsigned long long, int> sid2aid;

    /// mapping from gist Id to array Id (nodes_arr)
    std::tr1::unordered_map<unsigned long long, int> gid2aid;

    void show_db(void);

public Q_SLOTS:
    void startReading(void);
    
public:

    Data(TreeCanvas* tc, NodeAllocator* na, bool isRestarts);
    ~Data(void);

    static Data* self;

    bool readInstance(bool isRoot);
    void pushInstance(unsigned long long sid, DbEntry* entry);
    
    char* getLabelByGid(unsigned int gid);
    static int handleNodeCallback(Message* data);
    
};




#endif // DATA_HH

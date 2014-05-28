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
  DONE_SENDING = 2
};


struct Message {
    static const int LABEL_SIZE = 16;

    MsgType type;
    int sid;
    int parent_sid;
    int alt;
    int kids;
    int status;
    char thread;
    char label[16];
};

class DbEntry {
private:

public:
    DbEntry(int _p, int _alt, int _kids, char _tid, char* _label, int _status) :
        gid(-1), parent_sid(_p), alt(_alt), numberOfKids(_kids),
        thread(_tid), status(_status) {
          
          memcpy(label, _label, Message::LABEL_SIZE);
          std::cout << label << std::endl;
    }

    DbEntry(): gid(-1) {}

    int gid; // gist Id
    int parent_sid; // parent id in database 
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

    int counter;
    int lastRead;
    int firstIndex; // for nodes_arr

    int nextToRead = 0;
    int totalElements = -1;

    sqlite3 *db;
    TreeCanvas* _tc;
    NodeAllocator* _na;
    std::vector<DbEntry*> nodes_arr;

    /// mapping from solver Id to array Id (nodes_arr)
    std::tr1::unordered_map<int, int> sid2aid;

    /// mapping from gist Id to array Id (nodes_arr)
    std::tr1::unordered_map<int, int> gid2aid;

    void show_db(void);

    
    ~Data(void);

public Q_SLOTS:
    void startReading(void);
    
public:

    Data(TreeCanvas* tc, NodeAllocator* na);
    static Data* self;

    bool readInstance(NodeAllocator *na);
    void pushInstance(unsigned int sid, DbEntry* entry);
    char* getLabelByGid(unsigned int gid);
    static int handleNodeCallback(Message* data);
    
};




#endif // DATA_HH

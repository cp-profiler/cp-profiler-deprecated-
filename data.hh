#ifndef DATA_HH
#define DATA_HH

#include <vector>
#include <tr1/unordered_map>
#include <sqlite3.h>
#include <QTimer>

#include "treecanvas.hh"
#include "node.hh"

typedef NodeAllocatorBase<VisualNode> NodeAllocator;

class DbEntry {
private:

public:
    DbEntry(int _p, int _alt, int _kids, int _status) :
        node_id(-1), parent_id(_p), alt(_alt), numberOfKids(_kids), status(_status) {
    }

    DbEntry(): node_id(-1) {}

    int node_id; // id as it is in gist
    int parent_id; // parent id in database 
    int alt; // which child by order
    int numberOfKids;
    int status;
    char thread;

};

enum MsgType {
  NODE_DATA = 1,
  DONE_SENDING = 2
};

struct Message {
  MsgType type;
  int sid;
  int parent;
  int alt;
  int kids;
  int status;
  char thread;

  void specifyNode(int _sid, int _parent, int _alt, int _kids, int _status, char _thread) {
    type = NODE_DATA;
    sid = _sid;
    parent = _parent;
    alt = _alt;
    kids = _kids;
    status = _status;
    thread = _thread;
  }

};

class Data : public QObject {
Q_OBJECT
public:
    static const int READING_PERIOD = 1000;

private:

    static const int PORTION = 50000;

    int counter;
    int lastRead;
    int firstIndex; // for db_array

    int nextToRead = 0;
    int totalElements = -1;

    sqlite3 *db;
    TreeCanvas* _tc;
    NodeAllocator* _na;
    std::vector<DbEntry*> db_array;
    std::tr1::unordered_map<int, int> nid_to_db_id;

    void show_db(void);

    
    ~Data(void);

public Q_SLOTS:
    void startReading(void);
    
public:

    Data(TreeCanvas* tc, NodeAllocator* na);
    static Data* self;

    bool readInstance(NodeAllocator *na);
    void pushInstance(unsigned int sid, DbEntry* entry);
    static int handleNodeCallback(Message* data);
    
};




#endif // DATA_HH

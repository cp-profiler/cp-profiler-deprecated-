#ifndef DATA_HH
#define DATA_HH

#include <vector>
#include <tr1/unordered_map>
#include <sqlite3.h>
#include <QMutex>

#include "node.hh"

class DbEntry {
private:

public:
	DbEntry(int _p, int _alt, int _kids) :
		node_id(-1), parent_db_id(_p), alt(_alt), numberOfKids(_kids) {
	}

	DbEntry(): node_id(-1) {}

	int node_id; // id as it is in gist
	int parent_db_id; // parent id in database 
	int alt; // which child by order
	int numberOfKids;

};

class Data {

private:

	static const int PORTION = 4;
	int counter;
	int lastRead;

	sqlite3 *db;

	void show_db(void);
	void connectToDB(void);
	
	/// can I get rid of 'static' here?
    static int callback(void*, int argc, char **argv, char **azColName); 
	~Data(void);
    
public:
	void readDB(int db_id);
	typedef NodeAllocatorBase<VisualNode> NodeAllocator;

	QMutex* mutex;
	NodeAllocator * _na;
	static Data* self;
    std::vector<DbEntry*> db_array;
    std::tr1::unordered_map<int, int> nid_to_db_id;

    Data(NodeAllocator* na, QMutex* m);
    int specifyNId(Node::NodeAllocator &na, int db_id);
	int getKids(int nid);
	void startReading(void);
	bool readInstance(NodeAllocator *na);
	void printNA(void);
	
};




#endif // DATA_HH

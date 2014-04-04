#ifndef DATA_HH
#define DATA_HH

#include <vector>
#include <map>
#include <sqlite3.h>

#include "node.hh"

class DbEntry {
private:

public:
	DbEntry(int _p, int _alt, int _kids) :
		node_id(-1), parent_db_id(_p), alt(_alt), numberOfKids(_kids) {
	}

	int node_id; // id as it is in gist
	int parent_db_id; // parent id in database 
	int alt; // which child by order
	int numberOfKids;

};

class Data {

private:

	int counter;
	sqlite3 *db;

	void show_db(void);
	void connectToDB(void);
	void readInstance(int db_id);
	~Data(void);
    
public:
	typedef NodeAllocatorBase<VisualNode> NodeAllocator;

	static Data* self;
    std::vector<DbEntry*>* db_array;
    std::map<int, int> nid_to_db_id; 

    Data();
    int specifyNId(Node::NodeAllocator &na, int db_id);
	int getKids(int nid);
	int getIndex();
	
};




#endif // DATA_HH

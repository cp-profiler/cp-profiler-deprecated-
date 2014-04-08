#include <iostream>
#include <qdebug>
#include <string>
#include <sstream>
#include <sqlite3.h>
#include "visualnode.hh"


#include "data.hh"

using namespace std;

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

Data* Data::self = 0;

Data::Data() {
	counter = 0;
	Data::self = this;

 	// nid_to_db_id[0] = 0;
 	// nid_to_db_id[1] = 1;
 	// nid_to_db_id[3] = 2;
 	// nid_to_db_id[4] = 3;

 	// nid_to_db_id[5] = 4;
 	// nid_to_db_id[2] = 5;
 	// nid_to_db_id[10] = 6;
 	// nid_to_db_id[11] = 7;

 	// nid_to_db_id[6] = 8;
 	// nid_to_db_id[7] = 9;
 	// nid_to_db_id[8] = 10;
 	// nid_to_db_id[9] = 11;

 	// nid_to_db_id[12] = 12;
 	// nid_to_db_id[13] = 13;
 	// nid_to_db_id[14] = 14;
 	// nid_to_db_id[15] = 15;
  lastRead = -1;

 	connectToDB();
  qDebug() << "size: " << db_array.size();
  readDB(0);
  show_db();
}

int Data::specifyNId(NodeAllocator &na, int db_id) {

}

int Data::getKids(int nid) {

    int db_id = nid_to_db_id[nid];
    return db_array[db_id]->numberOfKids;
}

void Data::show_db(void) {
    for (vector<DbEntry*>::iterator it = db_array.begin(); it != db_array.end(); it++) {
        qDebug() << "nid: " << (*it)->node_id << " p: " << (*it)->parent_db_id <<
        " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
}

/// return false if there is nothing to read
bool Data::readInstance(NodeAllocator &na) {
  int nid;
  int parent_db_id;
  int parent_nid;
  int alt;
  int nalt;
  if (lastRead + 1 >= counter)
    return false;

  VisualNode* node;
  if (lastRead == -1) {
    lastRead++;
    na[0]->setNumberOfChildren(db_array[0]->numberOfKids, na);
    na[0]->setStatus(BRANCH);
    na[0]->setHasSolvedChildren(true);
    db_array[lastRead]->node_id = 0;
     
  }
  else {
     lastRead++;
     parent_db_id = db_array[lastRead]->parent_db_id;
     alt = db_array[lastRead]->alt;
     nalt = db_array[lastRead]->numberOfKids;
     parent_nid = db_array[parent_db_id]->node_id;
     node = na[parent_nid]->getChild(na, alt);
     db_array[lastRead]->node_id = node->getIndex(na);
     node->setNumberOfChildren(nalt, na);
     if (nalt > 0)
       node->setStatus(BRANCH);
     else
       node->setStatus(SOLVED);

  }

  return true;

}

void Data::connectToDB(void) {
	int rc;
  // rc = sqlite3_open("/Users/maxim/Dropbox/dev/StandaloneGist/data.db", &db);  
	rc = sqlite3_open("/Users/maxim/Dropbox/dev/StandaloneGist/InitialGist/StandaloneGist/data.db", &db);	

	if (rc) {
		qDebug() << "Can't connect to DB: " << sqlite3_errmsg(db);
	} else {
		qDebug() << "Successfully connected to DB";
	}

	

}

int Data::callback(void *NotUsed, int argc, char **argv, char **azColName) {
  int id, parent, alt, kids;
  int col_n = 5;
  for (int i = 0; i < argc; i += col_n) {
    Data::self->counter++;
    id = argv[i] ? atoi(argv[i]) : -1;
    parent = argv[i+2] ? atoi(argv[i+2]) : -1;
    alt = argv[i+3] ? atoi(argv[i+3]) : -1;
    kids = argv[i+4] ? atoi(argv[i+4]) : -1;
    Data::self->db_array.push_back(new DbEntry(parent, alt, kids));
  }
  
  return 0;
}

void Data::readDB(int db_id) {
  int rc;
  char *zErrMsg = 0;
  // string query = "SELECT * FROM Nodes WHERE Id=" + SSTR(db_id);
  string query = "SELECT * FROM Nodes WHERE Id>" + SSTR(db_id);
  query += ";";
  qDebug() << "command: " << query.c_str();
  rc = sqlite3_exec(db, query.c_str(), callback, 0, &zErrMsg);
  if (rc != SQLITE_OK ) {
    qDebug() << "SQL error: " << zErrMsg;
  } else 
    qDebug() << "query sent " << zErrMsg;
}

Data::~Data(void) {
  sqlite3_close(db);

  for (vector<DbEntry*>::iterator it = db_array.begin(); it != db_array.end(); it++) {
        delete (*it);
  }
}




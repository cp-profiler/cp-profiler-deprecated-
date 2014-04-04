#include <iostream>
#include <qdebug>
#include <sqlite3.h>

#include "data.hh"

using namespace std;

Data* Data::self = 0;

Data::Data() {
	counter = 0;
	Data::self = this;
	db_array = new std::vector<DbEntry*>();

   	db_array->push_back(new DbEntry(-1, -1, 2));
   	db_array->push_back(new DbEntry(0, 0, 2));
   	db_array->push_back(new DbEntry(1, 0, 0));
   	db_array->push_back(new DbEntry(1, 1, 2));

   	db_array->push_back(new DbEntry(3, 0, 0));
   	db_array->push_back(new DbEntry(0, 1, 1));
   	db_array->push_back(new DbEntry(5, 0, 2));
   	db_array->push_back(new DbEntry(6, 0, 0));

   	db_array->push_back(new DbEntry(3, 1, 3));
   	db_array->push_back(new DbEntry(8, 0, 0));
   	db_array->push_back(new DbEntry(8, 1, 0));
   	db_array->push_back(new DbEntry(8, 2, 0));

   	db_array->push_back(new DbEntry(6, 1, 3));
   	db_array->push_back(new DbEntry(12, 0, 0));
   	db_array->push_back(new DbEntry(12, 1, 0));
   	db_array->push_back(new DbEntry(12, 2, 0));

   	nid_to_db_id[0] = 0;
   	nid_to_db_id[1] = 1;
   	nid_to_db_id[3] = 2;
   	nid_to_db_id[4] = 3;

   	nid_to_db_id[5] = 4;
   	nid_to_db_id[2] = 5;
   	nid_to_db_id[10] = 6;
   	nid_to_db_id[11] = 7;

   	nid_to_db_id[6] = 8;
   	nid_to_db_id[7] = 9;
   	nid_to_db_id[8] = 10;
   	nid_to_db_id[9] = 11;

   	nid_to_db_id[12] = 12;
   	nid_to_db_id[13] = 13;
   	nid_to_db_id[14] = 14;
   	nid_to_db_id[15] = 15;

   	connectToDB();
    qDebug() << "size: " << db_array->size();

    show_db();
}

int Data::specifyNId(NodeAllocator &na, int db_id) {

}

int Data::getKids(int nid) {

    int db_id = nid_to_db_id[nid];
    return (*db_array)[db_id]->numberOfKids;
}

int Data::getIndex() {
	return ++counter;
}

void Data::show_db(void) {
    for (vector<DbEntry*>::iterator it = db_array->begin(); it != db_array->end(); it++) {
        qDebug() << "nid: " << (*it)->node_id << " p: " << (*it)->parent_db_id <<
        " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
}

void Data::connectToDB(void) {
	int rc;
	rc = sqlite3_open("/Users/maxim/Dropbox/dev/StandaloneGist/data.db", &db);	

	if (rc) {
		qDebug() << "Can't connect to DB: " << sqlite3_errmsg(db);
		sqlite3_close(db);
	} else {
		qDebug() << "Successfully connected to DB";
	}

	sqlite3_close(db);

}

void readInstance(int db_id) {

}

Data::~Data(void) {

}




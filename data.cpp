#include <iostream>
#include <qdebug>
#include <string>
#include <sstream>
#include <sqlite3.h>
#include <ctime>

#include "visualnode.hh"
#include "data.hh"

using namespace std;

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
                ( std::ostringstream() << std::dec << x ) ).str()

Data* Data::self = 0;

Data::Data(NodeAllocator *na) : _na(na) {
    Data::self = this;
    counter = 0;
    
    lastRead = -1;

    connectToDB();
    
}

int Data::specifyNId(NodeAllocator &na, int db_id) {

}

void Data::startReading(void) {
    qDebug() << "in startReading";
    
    readDB(0);
    sleep(1);
    readDB(4);
    // update();
    sleep(1);
    readDB(8);
    // update();
    sleep(1);
    readDB(12);
}

void Data::show_db(void) {
    for (vector<DbEntry*>::iterator it = db_array.begin(); it != db_array.end(); it++) {
            qDebug() << "nid: " << (*it)->node_id << " p: " << (*it)->parent_db_id <<
            " alt: " << (*it)->alt << " kids: " << (*it)->numberOfKids;
    }
}

/// return false if there is nothing to read
bool Data::readInstance(NodeAllocator *na) {
    int nid;
    int parent_db_id;
    int parent_nid;
    int alt;
    int nalt;

    if (lastRead +1 >= counter)
        return false;

    VisualNode* node;
    if (lastRead == -1) {
        lastRead++;
        (*na)[0]->setNumberOfChildren(db_array[0]->numberOfKids, *na);
        (*na)[0]->setStatus(BRANCH);
        (*na)[0]->setHasSolvedChildren(true);
        db_array[lastRead]->node_id = 0;
         
    }
    else {
        lastRead++;
        parent_db_id = db_array[lastRead]->parent_db_id;
        alt = db_array[lastRead]->alt;
        nalt = db_array[lastRead]->numberOfKids;
        parent_nid = db_array[parent_db_id]->node_id;
        node = (*na)[parent_nid]->getChild(*na, alt);
        db_array[lastRead]->node_id = node->getIndex(*na);
        node->setNumberOfChildren(nalt, *na);
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
    int col_n = 7;
    for (int i = 0; i < argc; i += col_n) {
        Data::self->counter++;
        id = argv[i] ? atoi(argv[i]) : -2;
        parent = argv[i+3] ? atoi(argv[i+3]) : -2;
        alt = argv[i+4] ? atoi(argv[i+4]) : -2;
        kids = argv[i+5] ? atoi(argv[i+5]) : -2;
        Data::self->db_array.push_back(new DbEntry(parent, alt, kids));


        // put readInstance here?
        Data::self->readInstance(Data::self->_na);
    }
    
    return 0;
}

void Data::readDB(int db_id) {
    int rc;
    char *zErrMsg = 0;
    string query = "SELECT * FROM Nodes WHERE Id>" + SSTR(db_id) + " AND Id<=" + SSTR(db_id + Data::PORTION);
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




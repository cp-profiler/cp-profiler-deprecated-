#include <iostream>
#include <QDebug>

#include "data.hh"

class DebugHelper {

public:
  static void printNodes(std::vector<DbEntry*> &nodes_arr) {
    for (auto it = nodes_arr.begin(); it != nodes_arr.end(); it++) {
      qDebug() << "gid:" << (*it)->gid << " parent_sid:" << (*it)->parent_sid;
    }
  }

  static void printMap(std::unordered_map<unsigned long long, int> &sid2aid) {
    for (auto it = sid2aid.begin(); it != sid2aid.end(); it++) {
      qDebug() << (*(it)).first << " -> " << (*(it)).second;
    }

  }


};

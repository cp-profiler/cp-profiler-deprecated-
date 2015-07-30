/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef READING_QUEUE
#define READING_QUEUE

#include <vector>
#include <map>
#include <queue>

class DbEntry;

typedef std::map<int, std::queue<DbEntry*>*> QueueMap;

class ReadingQueue {

private:

  /// nodes from Data
  std::vector<DbEntry*>& nodes_arr;

  /// nodes delayed, map: thread_id -> queue
  QueueMap delayed_treads;
  QueueMap::iterator it;

  unsigned last_read        = 0;       /// node id from nodes_arr currently read
  int delayed_count    = 0;       /// how many nodes delayed
  int delayed_cd_count = 0;       /// if zero, read delayed again
  const int DELAYED_CD = 1;     /// delayed cooldown


  /// how many nodes tried until success;
  /// increment if failed
  /// refresh on success
  int node_misses = 0;

  bool read_delayed    = false;


// inline DbEntry* nextNormal();
// inline DbEntry* nextDelayed();

  inline QueueMap::iterator nextNonemptyIt(QueueMap::iterator it);

public:



  ReadingQueue(std::vector<DbEntry*>& nodes);

  DbEntry* next(bool& delayed);

  /// whether nodes_arr is processed and all queues are empty
  bool canRead();

  /// notify regarding last processed entry
  void update(bool success);

  /// put into delayed queue
  void readLater(DbEntry* delayed);


};


#endif
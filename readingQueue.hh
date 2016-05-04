/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

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

  unsigned last_read = 0;    /// node id from nodes_arr currently read
  int delayed_count = 0;     /// how many nodes delayed
  int delayed_cd_count = 0;  /// if zero, read delayed again
  const int DELAYED_CD = 1;  /// delayed cooldown

  /// how many nodes tried until success;
  /// increment if failed
  /// refresh on success
  int node_misses = 0;

  bool read_delayed = false;

  inline QueueMap::iterator nextNonemptyIt(QueueMap::iterator it);

 public:
  explicit ReadingQueue(std::vector<DbEntry*>& nodes);

  DbEntry* next(bool& delayed);

  /// whether nodes_arr is processed and all queues are empty
  bool canRead();

  /// notify regarding last processed entry
  void update(bool success);

  /// put into delayed queue
  void readLater(DbEntry* delayed);
};

#endif
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

#include "readingQueue.hh"
#include "data.hh"
#include <iostream>

ReadingQueue::ReadingQueue(std::vector<DbEntry>& nodes)
: nodes_arr(nodes)
{

}

DbEntry&
ReadingQueue::next(bool& delayed) {

  /// for dubugging
  // std::cout << "*** delayed: ***\n";
  // for (auto it = delayed_treads.begin(); it != delayed_treads.end(); it++) {
  //   // std::cout << "front: " << (*it->second).front()->sid;
  //   std::cout << "size: " << (*it->second).size();
  //   std::cout << "\n";
  // }
  // std::cout << "*********\n";

  // std::cout << "nodes_arr: ";
  // for (auto entry = nodes_arr.begin(); entry != nodes_arr.end(); entry++) {
  //     std::cout << (*entry)->sid << " ";
  // }
  // std::cout << std::endl;

  /// if normal read mode && nodes_arr has unread elements
  if (!read_delayed && nodes_arr.size() > last_read) {

    /// come back to delayed anyway?
    if (delayed_count > 0 && delayed_cd_count <= 0){
      read_delayed = true;
      return next(delayed);
    }

    delayed = false;
    return nodes_arr[last_read++];
  } else {
    /// continue reading delayed or ran out of normal nodes

    /// option 1: can't read, try new queue
    if (it->second->size() == 0 || it == delayed_treads.end()) {
      it = nextNonemptyIt(++it);
      if (it == delayed_treads.end()) {
        std::cout << "no more delayed, reading normal nodes\n";
        read_delayed = false;
        /// recursion should not happen, because that would mean
        /// there is no more nodes to read, so next() would not
        /// be called in the first place
        return next(delayed);
      }
    }

    /// at this point there is nonempty queue at it->second
    delayed = true;
    read_delayed = true;
    return *it->second->front();
  }

}

QueueMap::iterator
ReadingQueue::nextNonemptyIt(QueueMap::iterator it) {
  while (it != delayed_treads.end()) {
    if (it->second->size() > 0) {
      // std::cout << "found delayed on tread: " << it->first << "\n";
      break;
    }
  }
  return it;
}


bool
ReadingQueue::canRead() {
  if (nodes_arr.size() != last_read || delayed_count > 0)
    return true;
  return false;
}

void
ReadingQueue::update(bool success) {
  /// if success on array -> continue array
  // if (!read_delayed && success) {
  //   std::cout << "successfully read, misses: " << node_misses << "\n";
  //   node_misses = 0;
  //   delayed_cd_count--;
  //   return;
  // }

  /// if success on delayed -> continue delayed
  if (read_delayed && success) {
    it->second->pop();
    delayed_count--;
    std::cout << "successfully read from delayed, "; 
    std::cout << delayed_count << " left ";
    std::cout << " misses: " << node_misses << "\n";
    return;
  }

  /// if failure on array -> switch to first nonempty queue of delayed
  if (!read_delayed && !success) {
    node_misses++;

    it = nextNonemptyIt(delayed_treads.begin()); /// start from the beginning
    /// if found -> read from that queue next
    if (it != delayed_treads.end()) {
      read_delayed = true;
      /// todo: remember the queue
    } else {
      std::cerr << "can't read from either queue, aborting\n";
      abort();
    }
    return;
  }

  /// if failure on delayed -> try another queue or switch to array
  if (read_delayed && !success) {
    node_misses++;
    it = nextNonemptyIt(++it);  /// start where left last time

    if (it != delayed_treads.end()) {
      /// todo: remember the queue
    } else {
      // std::cerr << "can't read delayed, trying normal nodes\n";
      delayed_cd_count = DELAYED_CD;
      read_delayed = false;
    }
    return;
  }

}

void
ReadingQueue::readLater(DbEntry& delayed) {
  int tid = delayed.thread;

  if (delayed_treads.find(tid) == delayed_treads.end()) {
      std::cout << "create delayed_treads[" << tid << "] queue\n";
      delayed_treads[tid] = new std::queue<DbEntry*>(); /// TODO: delete queues in the end
  } 

  /// delayed_treads[tid] exists at this point
  delayed_count++;
  delayed_treads[tid]->push(&delayed);
  // std::cout << "push " << *delayed_treads[tid]->front() << " into delayed_treads[" << tid << "]\n";
}
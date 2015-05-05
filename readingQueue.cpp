#include "readingQueue.hh"
#include "data.hh"
#include <iostream>

ReadingQueue::ReadingQueue(std::vector<DbEntry*>& nodes)
: nodes_arr(nodes), last_read(0), delayed_count(0), read_delayed(false)
{

}

DbEntry*
ReadingQueue::next(bool& delayed) {

  /// if normal read mode && nodes_arr has unread elements
  if (!read_delayed && nodes_arr.size() > last_read) {
    delayed = false;
    return nodes_arr[last_read++];
  } else {

    if (it->second->size() == 0) {
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
    return it->second->front();
  }

}

QueueMap::iterator
ReadingQueue::nextNonemptyIt(QueueMap::iterator it) {
  while (it != delayed_treads.end()) {
    if (it->second->size() > 0) {
      std::cout << "found delayed on tread: " << it->first << "\n";
      break;
    }
  }
  return it;
}


bool
ReadingQueue::canRead() {
  if (nodes_arr.size() != last_read)
    return true;
  return false;
}

void
ReadingQueue::update(bool success) {


  /// if success on array -> continue array
  if (!read_delayed && success) {
    std::cout << "successfully read\n";
    return;
  }

  /// if success on delayed -> continue delayed
  if (read_delayed && success) {
    std::cout << "successfully read from delayed, "; 
    std::cout << delayed_count << " left\n";
    it->second->pop();
    delayed_count--;
    return;
  }

  /// if failure on array -> switch to first nonempty queue of delayed
  if (!read_delayed && !success) {

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
    it = nextNonemptyIt(++it);  /// start where left last time

    if (it != delayed_treads.end()) {
      /// todo: remember the queue
    } else {
      std::cerr << "can't read delayed, trying normal nodes\n";
      read_delayed = false;
    }
    return;
  }

}

void
ReadingQueue::readLater(DbEntry* delayed) {
  int tid = delayed->thread;

  if (delayed_treads.find(tid) == delayed_treads.end()) {
      std::cout << "create delayed_treads[" << tid << "] queue\n";
      delayed_treads[tid] = new std::queue<DbEntry*>(); /// TODO: delete queues in the end
  } 

  /// delayed_treads[tid] exists at this point
  delayed_treads[tid]->push(delayed);
  std::cout << "push " << *delayed_treads[tid]->front() << " into delayed_treads[" << tid << "]\n";
}
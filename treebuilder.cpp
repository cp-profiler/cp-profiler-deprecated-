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

#include "treebuilder.hh"
#include "globalhelper.hh"
#include "libs/perf_helper.hh"
#include "readingQueue.hh"
#include "nodetree.hh"
#include <cassert>

TreeBuilder::TreeBuilder(Execution* exec, QObject* parent)
    : QThread(parent),
      execution(*exec),
      _data{execution.getData()},
      _na(execution.nodeTree().getNA()) {

  read_queue.reset(new ReadingQueue(_data.getEntries()));
}

TreeBuilder::~TreeBuilder() {}

bool TreeBuilder::processRoot(DbEntry& dbEntry) {
  QMutexLocker locker(&execution.getTreeMutex());
  QMutexLocker layoutLocker(&execution.getLayoutMutex());

  Statistics& stats = execution.getStatistics();

  stats.choices++;
  stats.undetermined += dbEntry.numberOfKids;

  // can be a real root, or one of initial nodes in restarts
  VisualNode* root = nullptr;

  if (execution.isRestarts()) {
#ifdef MAXIM_DEBUG
    qDebug() << "creating restart root";
#endif
    // create a node for a new root
    int restart_root = (_na)[0]->addChild(_na);
    root = (_na)[restart_root];

    // The "super root" now has an extra child, so its children
    // haven't been laid out yet.
    (_na)[0]->setChildrenLayoutDone(false);
    (_na)[0]->setHasOpenChildren(true);

    // The "super root" is effectively a branch node.
    (_na)[0]->setStatus(BRANCH);

    dbEntry.gid = restart_root;
    dbEntry.depth = 2;
  } else {
    root = (_na)[0];  // use the root that is already there
    if (root->getStatus() != UNDETERMINED) {
      std::cout << dbEntry << std::endl;
      return true;
    }
    dbEntry.gid = 0;
    dbEntry.depth = 1;
  }

  auto& gid2entry = _data.gid2entry;
  gid2entry[dbEntry.gid] = &dbEntry;

  /// setNumberOfChildren
  root->setNumberOfChildren(dbEntry.numberOfKids, _na);
  root->setStatus(BRANCH);
  root->setHasSolvedChildren(false);
  root->setHasOpenChildren(true);

  root->dirtyUp(_na);

  emit addedRoot();
  emit addedNode();

  return true;
}

bool TreeBuilder::processNode(DbEntry& dbEntry, bool is_delayed) {
  QMutexLocker locker(&execution.getTreeMutex());
  QMutexLocker layoutLocker(&execution.getLayoutMutex());

  NodeUID p_uid = dbEntry.parentUID;  /// parent ID as it comes from Solver
  int alt = dbEntry.alt;             /// which alternative the current node is
  int nalt = dbEntry.numberOfKids;   /// number of kids in current node
  char status = dbEntry.status;

  /// find out if node exists
  auto pid_it = _data.uid2aid.find(p_uid);

  if (pid_it == _data.uid2aid.end()) {
    if (!is_delayed) {
      read_queue->readLater(&dbEntry);
    }

    return false;
  }

  const DbEntry& parentEntry = *_data.getEntries().at(pid_it->second);
  /// parent ID as it is in Node Allocator (Gist)
  int parent_gid = parentEntry.gid;

  /// put delayed also if parent node hasn't been processed yet:
  if (parent_gid == -1) {

    if (!is_delayed)
      read_queue->readLater(&dbEntry);
    else {
      // qDebug() << "node already in the queue";
    }

    return false;
  }

  auto parent = (_na)[parent_gid];

  if (!parent) {
#ifdef MAXIM_DEBUG
    std::cerr << "can't parse node" << dbEntry;
#endif
    ignored_entries.push_back(&dbEntry);
    return false;
  }

  auto& stats = execution.getStatistics();

  while (alt >=  parent->getNumberOfChildren()) {
    parent->addChild(_na);
    stats.undetermined++;
  }

  auto& node = *parent->getChild(_na, alt);

  /// Normal behaviour: insert into Undetermined
  if (node.getStatus() == UNDETERMINED) {
    stats.undetermined--;

    int gid = node.getIndex(_na);  // node ID as it is in Gist

    /// fill in empty fields of dbEntry
    dbEntry.gid = gid;
    dbEntry.depth = parentEntry.depth + 1;  /// parent's depth + 1

    // For now, assume that the solver sends the decision level.

    // // It is not always clear how to compute a node's decision
    // // level with respect to its parent.  For now, let us say that
    // // the right-most branch is not a decision, and all other
    // // branches are decisions.
    // bool thisIsRightmost = (alt == parentEntry.numberOfKids - 1);
    // dbEntry.decisionLevel =
    //     parentEntry.decisionLevel + (thisIsRightmost ? 0 : 1);

    _data.gid2entry[gid] = &dbEntry;

    stats.maxDepth = std::max(stats.maxDepth, static_cast<int>(dbEntry.depth));

    /// NOTE: don't distinguish between -1 and 0
    /// -1 is the default for Chuffed and 0 -- for Gecode
    if (dbEntry.thread_id == -1) { dbEntry.thread_id = 0; }

    node._tid = dbEntry.thread_id;  /// TODO: tid should be in node's flags

    node.setNumberOfChildren(nalt, _na);

    switch (status) {
      case FAILED:  // 1
        node.setHasOpenChildren(false);
        node.setHasSolvedChildren(false);
        node.setHasFailedChildren(true);
        node.setStatus(FAILED);
        parent->closeChild(_na, true, false);
        stats.failures++;

        break;
      case SKIPPED:  // 6
        // check if node hasn't been explored by other thread
        // (for now just check if failure node)
        node.setHasOpenChildren(false);
        node.setHasSolvedChildren(false);
        node.setHasFailedChildren(true);
        node.setStatus(SKIPPED);
        parent->closeChild(_na, true, false);
        stats.failures++;
        break;
      case SOLVED:  // 0
        node.setHasFailedChildren(false);
        node.setHasSolvedChildren(true);
        node.setHasOpenChildren(false);
        node.setStatus(SOLVED);
        parent->closeChild(_na, false, true);
        stats.solutions++;
        break;
      case BRANCH:  // 2
        node.setHasOpenChildren(true);
        node.setStatus(BRANCH);
        stats.choices++;
        stats.undetermined += nalt;
        break;
      default:
        qDebug() << "need to handle this type of Node: " << status;
        break;
    }

    node.dirtyUp(_na);
    emit addedNode();
    // std::cerr << "TreeBuilder::processNode, normal case\n";
  } else {
    /// Not normal cases:
    /// 1. Branch Node or Failed node into Skipped

    if (node.getStatus() == SKIPPED) {
      switch (status) {
        case FAILED:  // 1
          node.setHasOpenChildren(false);
          node.setHasSolvedChildren(false);
          node.setHasFailedChildren(true);
          node.setStatus(FAILED);
          parent->closeChild(_na, true, false);
          stats.failures++;

          break;
        case BRANCH:  // 2
          node.setHasOpenChildren(true);
          node.setStatus(BRANCH);
          stats.choices++;
          stats.undetermined += nalt;
          break;
        default:
          qDebug() << "need to handle this type of Node: " << status;
          assert(status != SOLVED);
          break;
      }
      node.dirtyUp(_na);
      emit addedNode();
      // std::cerr << "TreeBuilder::processNode, not-normal case\n";
    } else {
      // assert(status == SKIPPED);
      ignored_entries.push_back(&dbEntry);
      /// sometimes branch wants to override branch
    }
  }

  return true;
}

void TreeBuilder::run() {

  perf_helper::Timer timer;
  timer.begin();

  QMutex& dataMutex = _data.dataMutex;

  bool is_delayed;

  while (true) {

    dataMutex.lock();
    /// check if done
    if (!read_queue->canRead()) {
      dataMutex.unlock();

      if (_data.isDone()) {
        break;
      }
      /// can't read, but receiving not done, waiting...
      msleep(1);
      continue;
    }

    /// ask queue for an entry, note: is_delayed gets assigned here
    DbEntry* entry = read_queue->next(is_delayed);

    bool isRoot = (entry->parentUID.nid == -1) ? true : false;

    /// try to put node into the tree
    bool success = isRoot ? processRoot(*entry) : processNode(*entry, is_delayed);
    read_queue->update(success);

    dataMutex.unlock();
  }

  emit doneBuilding(true);

  auto elapsed_ms = timer.end();
  std::cout << "Time elapsed: " << elapsed_ms << "ms\n";
  if (GlobalParser::isSet(GlobalParser::test_option)) {
    qDebug() << "test mode, terminate";
    qApp->exit();
  }
}

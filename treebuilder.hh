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

#ifndef TREEBUILDER_H
#define TREEBUILDER_H

#include <QtGui>
#include <vector>
#include <queue>
#include "data.hh"
#include "execution.hh"
#include <memory>

class Data;
class DbEntry;
class Execution;
class ReadingQueue;
class TreeCanvas;
class NodeAllocator;

enum NodeStatus : char;

class TreeBuilder : public QThread {
  Q_OBJECT

 private:
  Execution& execution;
  Data& _data; /// Note: mutable as builder changes dbEntries
  NodeAllocator& _na;

  std::vector<DbEntry*> ignored_entries;

  std::unique_ptr<ReadingQueue> read_queue;

  bool processRoot(DbEntry& dbEntry);
  bool processNode(DbEntry& dbEntry, bool is_delayed);

  void run() override;

  void initRoot(int kids, NodeStatus status);

 public:
  TreeBuilder(Execution* execution, QObject* parent = nullptr);
  ~TreeBuilder();

Q_SIGNALS:
  void doneBuilding(bool finished);
  void addedNode(void);
  void addedRoot(void);
};

#endif  // TREEBUILDER_H

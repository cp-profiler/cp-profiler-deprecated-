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
#include "treecanvas.hh"
#include "readingQueue.hh"

typedef NodeAllocatorBase<VisualNode> NodeAllocator;

class Data;
class DbEntry;

class TreeBuilder : public QThread {
    Q_OBJECT

private:
	Data* _data;
	NodeAllocator* _na;
	TreeCanvas* _tc;
	QMutex* layout_mutex;

	unsigned long long lastRead;
    int delayed_count = 0;
    int nodesCreated;

    std::vector<DbEntry*> ignored_entries;
    std::vector<DbEntry*> processed;
    

    ReadingQueue* read_queue;

private:

    inline bool processRoot(DbEntry& dbEntry);
    inline bool processNode(DbEntry& dbEntry, bool is_delayed);

public:
    TreeBuilder(TreeCanvas* tc, QObject *parent = 0);
    ~TreeBuilder();
    void reset(Data* data, NodeAllocator *na);

Q_SIGNALS:
	void doneBuilding(bool finished);

public Q_SLOTS:
    void startBuilding(void);
    void setDoneReceiving(void);

protected:
    void run();
};

#endif // TREEBUILDER_H

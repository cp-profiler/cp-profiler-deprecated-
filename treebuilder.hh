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

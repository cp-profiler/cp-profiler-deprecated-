#ifndef TREEBUILDER_H
#define TREEBUILDER_H

#include <QtGui>
#include "data.hh"
#include "treecanvas.hh"

typedef NodeAllocatorBase<VisualNode> NodeAllocator;

class DbEntry;

class TreeBuilder : public QThread {
    Q_OBJECT

private:
	Data* _data;
	NodeAllocator* _na;
	TreeCanvas* _tc;
	QMutex* _mutex;

	unsigned long long lastRead;
    int nodesCreated;

private:

    inline void processRoot(DbEntry& dbEntry);
    inline void processNode(DbEntry& dbEntry);

public:
    TreeBuilder(TreeCanvas* tc, QObject *parent = 0);
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

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

	long long lastRead;

public:
    TreeBuilder(TreeCanvas* tc, QObject *parent = 0);
    void reset(Data* data, NodeAllocator *na);

public Q_SLOTS:
    void startBuilding();

protected:
    void run();
};

#endif // TREEBUILDER_H

#include "treebuilder.hh"

TreeBuilder::TreeBuilder(TreeCanvas* tc, QObject *parent) 
    : QThread(parent), _tc(tc) {
    	// lastRead = -1;
        qDebug() << "new Tree Builder";
}

void TreeBuilder::startBuilding() {
    qDebug() << "in startBuilding!";
    
    start();
}

void TreeBuilder::reset(Data* data, NodeAllocator* na) {
	_data = data;
	_na = na;
    lastRead = 0;
}

void TreeBuilder::run(void) {

	qDebug() << "in run method";

	std::vector<DbEntry*> &nodes_arr = _data->nodes_arr;
	std::unordered_map<unsigned long long, int> &sid2aid = _data->sid2aid;
	std::unordered_map<unsigned long long, int> &gid2aid = _data->gid2aid;

    bool _isRestarts = _data->_isRestarts;

    VisualNode* node;
    VisualNode* parent;

    while(true) {

        int gid; /// gist Id
        unsigned long long pid;
        int parent_gid = -2; /// gist Id of parent
        int alt;
        int nalt;
        int status;

        if (lastRead >= nodes_arr.size()) {
            if (Data::self->isDone())
                break;
            else {
                qDebug() << "continue...";
                continue;

            }
                
        }

        bool isRoot = (nodes_arr[lastRead]->parent_sid == ~0u) ? true : false;

        // qDebug() << "isRoot: " << isRoot;
        // qDebug() << "parent_sid: " << nodes_arr[lastRead]->parent_sid;

        if (isRoot) {       
            if (_isRestarts) {
                int restart_root = (*_na)[0]->addChild(*_na); // create a node for a new root
                qDebug() << "restart_root_id: " << restart_root;
                VisualNode* new_root = (*_na)[restart_root];
                new_root->setNumberOfChildren(nodes_arr[lastRead]->numberOfKids, *_na);
                new_root->setStatus(BRANCH);
                new_root->setHasSolvedChildren(true);
                new_root->_tid = nodes_arr[lastRead]->thread;
                nodes_arr[lastRead]->gid = restart_root;

            } else {
                (*_na)[0]->setNumberOfChildren(nodes_arr[0]->numberOfKids, *_na);
                (*_na)[0]->setStatus(BRANCH);
                (*_na)[0]->setHasSolvedChildren(true);
                (*_na)[0]->_tid = 0; /// thread id
                nodes_arr[lastRead]->gid = 0;
            }  
        }
        else { /// not a root

            pid = nodes_arr[lastRead]->parent_sid;
            // show_db();

            alt = nodes_arr[lastRead]->alt;
            nalt = nodes_arr[lastRead]->numberOfKids;
            status = nodes_arr[lastRead]->status;
            parent_gid = nodes_arr[sid2aid[pid]]->gid;
            parent = (*_na)[parent_gid];
            node = parent->getChild(*_na, alt);
            gid = node->getIndex(*_na);						// Gist ID
            nodes_arr[lastRead]->gid = gid;
            gid2aid[gid] = lastRead;

            qDebug() << "[" << lastRead - 1  << parent_gid << "]";

            node->_tid = nodes_arr[lastRead]->thread;
            node->setNumberOfChildren(nalt, *_na);

            switch (status) {
                case FAILED: // 1
                    node->setHasOpenChildren(false);
                    node->setHasSolvedChildren(false);
                    node->setHasFailedChildren(true);
                    node->setStatus(FAILED);
                    parent->closeChild(*_na, true, false);
                    _tc->stats.failures++;
                break;
                case SKIPPED: // 6
                    node->setHasOpenChildren(false);
                    node->setHasSolvedChildren(false);
                    node->setHasFailedChildren(true);
                    node->setStatus(SKIPPED);
                    parent->closeChild(*_na, true, false);
                    _tc->stats.failures++;
                break;
                case SOLVED: // 0
                    node->setHasFailedChildren(false);
                    node->setHasSolvedChildren(true);
                    node->setHasOpenChildren(false);
                    node->setStatus(SOLVED);
                    parent->closeChild(*_na, false, true);
                    _tc->stats.solutions++;
                break;
                case BRANCH: // 2
                    node->setHasOpenChildren(true);
                    node->setStatus(BRANCH);
                    _tc->stats.choices++;
                break;

            }

            /// do I need a mutex here?
                
            static_cast<VisualNode*>(node)->changedStatus(*_na);
            node->dirtyUp(*_na);
            qDebug() << "dirty up: " << node->getIndex(*_na);

        }

        lastRead++;

    }

    qDebug() << "lastRead: " << lastRead;
    node->dirtyUp(*_na);
    emit doneBuilding();
    qDebug() << "Done building";

}

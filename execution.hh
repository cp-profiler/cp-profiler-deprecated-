#ifndef EXECUTION_HH
#define EXECUTION_HH

#include <QObject>
#include <QDebug>
#include <sstream>
#include <ctime>
#include <memory>
#include "nodetree.hh"
#include <unordered_map>
#include "data.hh"

class Data;
class DbEntry;
class NodeAllocator;

namespace message {
    class Node;
}
// <<<<<<< HEAD
// ||||||| merged common ancestors
// =======
// 

class TreeBuilder;
// >>>>>>> master

class Execution : public QObject {
    Q_OBJECT

    friend class TreeCanvas;
public:
    Execution();

    const std::string* getNogood(const Node& node) const {
        auto entry = getEntry(node);
        if (!entry) return nullptr;
        auto nogood = _data->sid2nogood.find(entry->full_sid);
        if (nogood == _data->sid2nogood.end()) return nullptr;
        return &nogood->second;
    }

    const std::string* getInfo(const Node& node) const {
        auto entry = getEntry(node);
        if (!entry) return nullptr;
        auto info = _data->sid2info.find(entry->s_node_id);
        if (info == _data->sid2info.end()) return nullptr;
        return info->second;
    }

    std::string getTitle() { return _data->getTitle(); }
    std::string getDescription() {
        std::stringstream ss;
        /// TODO: also print model name (from _data->getTitle() -- comes with the first node)
        // ss << "an execution with " << _data->size() << " nodes";
        ss << getTitle();
        return ss.str();
    }
    
    DbEntry* getEntry(const Node& node) const {
        auto gid = node.getIndex(getNA());
        return getEntry(gid);
    }
    const NodeAllocator& getNA() const {
        return node_tree.getNA();
    }
    NodeAllocator& getNA() {
        return node_tree.getNA();
    }

    const NodeTree& nodeTree() const {
        return node_tree;
    }

    const std::unordered_map<int64_t, std::string>& getNogoods() const;
    std::unordered_map<int64_t, std::string*>& getInfo(void) const;
    DbEntry* getEntry(int gid) const;
    unsigned int getGidBySid(int sid);
    std::string getLabel(int gid) const;
    unsigned long long getTotalTime();
    std::string getTitle() const;

    Data* getData() const;

    void start(std::string label, bool isRestarts);

    bool isDone() const { return _is_done; }

    VisualNode* getRootNode() const {
        return node_tree.getRootNode();
    }

    Statistics& getStatistics() {
        return node_tree.getStatistics();
    }

    QMutex& getMutex() { return node_tree.getMutex(); }
    QMutex& getLayoutMutex() { return node_tree.getLayoutMutex(); }

signals:
    void newNode();
    void newRoot();
    void titleKnown();
    void startReceiving();
    void doneReceiving();
    void doneBuilding();

private:
    // TODO(maxim): figure out how to get away without
    // including data.hh
    std::unique_ptr<Data> _data;
    NodeTree node_tree;
    bool _is_done;
    TreeBuilder* builder;
public Q_SLOTS:
    void handleNewNode(message::Node& node);

};

#endif

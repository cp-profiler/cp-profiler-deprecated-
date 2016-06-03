#ifndef EXECUTION_HH
#define EXECUTION_HH

#include <QObject>
#include <QDebug>
#include "data.hh"
#include <sstream>
#include <ctime>
#include <memory>
#include "nodetree.hh"
#include "treebuilder.hh"

class TreeBuilder;

class Execution : public QObject {
    Q_OBJECT

    friend class TreeCanvas;
public:
    Execution() {
        _data = std::unique_ptr<Data>{new Data()};
        builder = NULL;
        _is_done = false;
    }

    inline const std::unordered_map<int64_t, string>& getNogoods(void) const { return _data->getNogoods(); }
    const std::string* getNogood(const Node& node) const {
        auto entry = getEntry(node);
        if (!entry) return nullptr;
        auto nogood = _data->sid2nogood.find(entry->full_sid);
        if (nogood == _data->sid2nogood.end()) return nullptr;
        return &nogood->second;
    }

    inline std::unordered_map<int64_t, string*>& getInfo(void) const { return _data->getInfo(); }
    const std::string* getInfo(const Node& node) const {
        auto entry = getEntry(node);
        if (!entry) return nullptr;
        auto info = _data->sid2info.find(entry->s_node_id);
        if (info == _data->sid2info.end()) return nullptr;
        return info->second;
    }
    DbEntry* getEntry(int gid) const { return _data->getEntry(gid); }
    DbEntry* getEntry(const Node& node) const {
        auto gid = node.getIndex(getNA());
        return getEntry(gid);
    }
    const NodeAllocator& getNA() const {
        return nodeTree.getNA();
    }
    NodeAllocator& getNA() {
        return nodeTree.getNA();
    }
    unsigned int getGidBySid(int sid) { return _data->getGidBySid(sid); }
    std::string getLabel(int gid) const { return _data->getLabel(gid); }
    unsigned long long getTotalTime() { return _data->getTotalTime(); }
    string getTitle() { return _data->getTitle(); }

    string getDescription() {
        std::stringstream ss;
        /// TODO: also print model name (from _data->getTitle() -- comes with the first node)
        // ss << "an execution with " << _data->size() << " nodes";
        ss << getTitle();
        return ss.str();
    }


    Data* getData() const {
        return _data.get();
    }

    void start(std::string label, bool isRestarts);

    bool isDone() const {
        return _is_done;
    }

    VisualNode* getRootNode() const {
        return nodeTree.getRootNode();
    }

    Statistics& getStatistics() {
        return nodeTree.getStatistics();
    }

    QMutex& getMutex() { return nodeTree.getMutex(); }
    QMutex& getLayoutMutex() { return nodeTree.getLayoutMutex(); }

signals:
    void newNode();
    void newRoot();
    void titleKnown();
    void startReceiving();
    void doneReceiving();
    void doneBuilding();

private:
    std::unique_ptr<Data> _data;
    NodeTree nodeTree;
    bool _is_done;
    TreeBuilder* builder;
public Q_SLOTS:
    void handleNewNode(message::Node& node) {
        // std::cerr << "execution::newNode\n";
        _data->handleNodeCallback(node);
    }
};

#endif

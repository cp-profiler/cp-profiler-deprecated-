#ifndef EXECUTION_HH
#define EXECUTION_HH

#include <QObject>
#include <QDebug>
#include <sstream>
#include <ctime>
#include <memory>
#include "nodetree.hh"
#include <unordered_map>

class Data;
class DbEntry;
class NodeAllocator;

namespace message {
    class Node;
}

class TreeBuilder;

class Execution : public QObject {
    Q_OBJECT

public:
    Execution();
    Execution(const Execution&) = delete;
    Execution& operator=(const Execution&) = delete;
    ~Execution();

    const std::string* getNogood(const Node& node) const;
    const std::string* getInfo(const Node& node) const;

    std::string getTitle() const;
    std::string getDescription() {
        std::stringstream ss;
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

    Data* getData() const;

    void start(std::string label, bool isRestarts);

    bool isDone() const { return _is_done; }
    bool isRestarts() const { return _is_restarts; }

    VisualNode* getRootNode() const {
        return node_tree.getRootNode();
    }

    Statistics& getStatistics() {
        return node_tree.getStatistics();
    }

    QMutex& getMutex() { return node_tree.getMutex(); }
    QMutex& getLayoutMutex() { return node_tree.getLayoutMutex(); }

    void setVariableListString(const std::string& s) {
        variableListString = s;
        // std::cerr << "set variableListString to " << s << "\n";
    }

    std::string getVariableListString(void) {
        return variableListString;
    }

signals:
    void newNode();
    void newRoot();
    void titleKnown();
    void startReceiving();
    void doneReceiving();
    void doneBuilding();

private:
    std::unique_ptr<Data> m_Data;
    std::unique_ptr<TreeBuilder> m_Builder;
    NodeTree node_tree;
    bool _is_done = false;
    bool _is_restarts;
    std::string variableListString;
public Q_SLOTS:
    void handleNewNode(message::Node& node);

};

#endif

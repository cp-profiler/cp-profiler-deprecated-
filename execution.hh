#ifndef EXECUTION_HH
#define EXECUTION_HH

#include <QObject>
#include <QDebug>
#include <sstream>
#include <ctime>
#include <memory>
// #include "nodetree.hh"
#include <unordered_map>
#include <condition_variable>

class Data;
class NameMap;
class DbEntry;
class NodeAllocator;
class NodeTree;
class Node;
class VisualNode;
class Statistics;
class QMutex;

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

    /// create an execution with an existing NodeTree and Data
    Execution(std::unique_ptr<NodeTree> nt, std::unique_ptr<Data> data);

    ~Execution();

    const std::string* getNogood(const Node& node) const;
    const std::string* getInfo(const Node& node) const;

    int getExecutionId() const {
        return execution_id;
    }
    void setExecutionId(int eid) {
        execution_id = eid;
    }


    std::string getTitle() const { return _title; }
    void setTitle(std::string title) { _title = title; }
    std::string getDescription() {
        std::stringstream ss;
        ss << getTitle();
        return ss.str();
    }
    
    DbEntry* getEntry(int gid) const;
    DbEntry* getEntry(const Node& node) const;

    const NodeTree& nodeTree() const { return *m_NodeTree.get(); }

    NodeTree& nodeTree() { return *m_NodeTree.get(); }

    const std::unordered_map<int, std::string>& getNogoods() const;
    std::string getNogoodBySid(int sid) const;
    std::unordered_map<int64_t, std::string*>& getInfo(void) const;
    unsigned getGidBySid(int64_t sid);

    std::string getLabel(int gid) const;
    std::string getLabel(const VisualNode& node) const;

    unsigned long long getTotalTime();

    Data& getData() const;

    void start(std::string label, bool isRestarts);

    bool isRestarts() const { return _is_restarts; }

    Statistics& getStatistics();

    QMutex& getMutex();
    QMutex& getLayoutMutex();

    void setVariableListString(const std::string& s) {
        variableListString = s;
    }

    std::string getVariableListString() {
        return variableListString;
    }

    void setNameMap(NameMap* names);
    const NameMap* getNameMap() const;

    bool finished{false};
    std::condition_variable has_execution_id;

public slots:
    void handleNewNode(message::Node& node);
    /// Compare domains of two nodes (highlighted)
    void compareDomains();

signals:
    void newNode();
    void newRoot();
    void titleKnown();
    void doneReceiving();
    void doneBuilding();

private:
    std::unique_ptr<NodeTree> m_NodeTree;
    std::unique_ptr<Data> m_Data;
    std::unique_ptr<TreeBuilder> m_Builder;

    int execution_id;
    bool _is_restarts;
    // Name of the FlatZinc model
    std::string _title = "";
    std::string variableListString;
};

#endif

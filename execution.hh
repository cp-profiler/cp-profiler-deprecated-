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

using NameMap = std::unordered_map<std::string, std::string>;
std::string replaceNames(const NameMap& nameMap, const std::string& text);

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
    void setNameMap(std::unordered_map<std::string, std::string>& names) {
        nameMap = names;
    }

    std::string getTitle() const { return _title; }
    void setTitle(std::string title) { _title = title; }
    std::string getDescription() {
        std::stringstream ss;
        ss << getTitle();
        return ss.str();
    }
    
    DbEntry* getEntry(const Node& node) const {
        auto gid = node.getIndex(m_NodeTree->getNA());
        return getEntry(gid);
    }

    const NodeTree& nodeTree() const { return *m_NodeTree.get(); }

    NodeTree& nodeTree() { return *m_NodeTree.get(); }

    const std::unordered_map<int64_t, std::string>& getNogoods() const;
    std::unordered_map<int64_t, std::string*>& getInfo(void) const;
    DbEntry* getEntry(int gid) const;
    unsigned int getGidBySid(int sid);
    std::string getLabel(int gid) const;

    std::string getLabel(const VisualNode& node) const {
        auto gid = node.getIndex(m_NodeTree->getNA());
        return getLabel(gid);
    }
    unsigned long long getTotalTime();

    Data& getData() const;

    void start(std::string label, bool isRestarts);

    bool isRestarts() const { return _is_restarts; }

    Statistics& getStatistics() {
        return m_NodeTree->getStatistics();
    }

    QMutex& getMutex() { return m_NodeTree->getMutex(); }
    QMutex& getLayoutMutex() { return m_NodeTree->getLayoutMutex(); }

    void setVariableListString(const std::string& s) {
        variableListString = s;
    }

    std::string getVariableListString() {
        return variableListString;
    }

    const NameMap& getNameMap() const {
        return nameMap;
    }

public slots:
    void handleNewNode(message::Node& node);

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
    NameMap nameMap;
    bool _is_restarts;
    // Name of the FlatZinc model
    std::string _title = "";
    std::string variableListString;


};

#endif

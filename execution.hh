#ifndef EXECUTION_HH
#define EXECUTION_HH

#include <QObject>
#include <QDebug>
#include <sstream>
#include <ctime>
#include <memory>
#include <unordered_map>
#include "data.hh"

class Data;
class DbEntry;
class NodeAllocator;

namespace message {
    class Node;
}

class Execution : public QObject {
    Q_OBJECT

    friend class TreeCanvas;
public:
    Execution();

    const std::unordered_map<int64_t, std::string>& getNogoods() const;
    std::unordered_map<int64_t, std::string*>& getInfo(void) const;
    DbEntry* getEntry(int gid) const;
    unsigned int getGidBySid(int sid);
    std::string getLabel(int gid) const;
    unsigned long long getTotalTime();
    std::string getTitle() const;

    Data* getData() const;

    NodeAllocator& na() const;

    void start(std::string label, bool isRestarts);

    bool isDone() const { return _is_done; }


signals:
    void newNode();
    void titleKnown();
    void startReceiving();
    void doneReceiving();
    void doneBuilding();

private:
    // TODO(maxim): figure out how to get away without
    // including data.hh
    std::unique_ptr<Data> _data;
    std::unique_ptr<NodeAllocator> _na;
    bool _is_done;
public Q_SLOTS:
    void handleNewNode(message::Node& node);
};

#endif

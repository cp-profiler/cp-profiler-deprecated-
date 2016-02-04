#ifndef EXECUTION_HH
#define EXECUTION_HH

#include <QObject>
#include "data.hh"
#include <sstream>

class Execution : public QObject {
    Q_OBJECT
public:
    Execution() {
        NodeAllocator* na = new NodeAllocator(false);
        _data = new Data(na, true);
        connect(this, SIGNAL(doneReceiving(void)), _data, SLOT(setDoneReceiving(void)));
    }

    inline const std::unordered_map<unsigned long long, string>& getNogoods(void) { return _data->getNogoods(); }
    inline std::unordered_map<unsigned long long, string>& getInfo(void) { return _data->getInfo(); }
    DbEntry* getEntry(unsigned int gid) const { return _data->getEntry(gid); }
    unsigned int getGidBySid(unsigned int sid) { return _data->getGidBySid(sid); }
    std::string getLabel(unsigned int gid) { return _data->getLabel(gid); }
    unsigned long long getTotalTime() { return _data->getTotalTime(); }
    string getTitle() { return _data->getTitle(); }

    string getDescription() {
        std::stringstream ss;
        /// TODO: also print model name (from _data->getTitle() -- comes with the first node)
        // ss << "an execution with " << _data->size() << " nodes";
        ss << getTitle();
        return ss.str();
    }


    Data* getData() {
        return _data;
    }

    void start(std::string label) {
        _data->setTitle(label);
        emit titleKnown();
    }


signals:
    void newNode();
    void titleKnown();
    void startReceiving();
    void doneReceiving();
private:
    Data* _data;
public Q_SLOTS:
    void handleNewNode(message::Node& node) {
        // std::cerr << "execution::newNode\n";
        _data->handleNodeCallback(node);

        emit newNode();
    }
};

#endif

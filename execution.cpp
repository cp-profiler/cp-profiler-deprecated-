#include "execution.hh"
#include "data.hh"

using std::string;

Execution::Execution() {
	_na = std::unique_ptr<NodeAllocator>{new NodeAllocator{}};
	_data = std::unique_ptr<Data>{new Data(_na.get())};
}

Data* Execution::getData() const {
    return _data.get();
}

NodeAllocator& Execution::na() const {
    return *_na.get();
}

void Execution::start(std::string label, bool isRestarts) {

    _data->setIsRestarts(isRestarts);

    std::time_t t = std::time(nullptr);
    string ts = std::asctime(std::localtime(&t));

    // asctime puts a newline at the end; remove it
    ts.pop_back();
    _data->setTitle(label + " (" + ts + ")");

    connect(this, SIGNAL(doneReceiving(void)), _data.get(), SLOT(setDoneReceiving(void)));
    connect(this, &Execution::doneReceiving, [this](){ _is_done = true; });

    emit titleKnown();
}

void Execution::handleNewNode(message::Node& node) {
    // std::cerr << "execution::newNode\n";
    _data->handleNodeCallback(node);

    emit newNode();
}

const std::unordered_map<int64_t, string>& Execution::getNogoods() const {
  return _data->getNogoods();
}
std::unordered_map<int64_t, string*>& Execution::getInfo(void) const {
  return _data->getInfo();
}
DbEntry* Execution::getEntry(int gid) const { return _data->getEntry(gid); }
unsigned int Execution::getGidBySid(int sid) { return _data->getGidBySid(sid); }
std::string Execution::getLabel(int gid) const { return _data->getLabel(gid); }
unsigned long long Execution::getTotalTime() { return _data->getTotalTime(); }
string Execution::getTitle() const { return _data->getTitle(); }
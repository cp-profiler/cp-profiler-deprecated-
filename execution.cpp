#include "execution.hh"
#include "data.hh"
#include "treebuilder.hh"

using std::string;

Execution::Execution() {
	_data = std::unique_ptr<Data>{new Data()};
}

Execution::~Execution() = default;

Data* Execution::getData() const {
    return _data.get();
}

void Execution::start(std::string label, bool isRestarts) {
    _data->setIsRestarts(isRestarts);

    std::time_t t = std::time(nullptr);
    string ts = std::asctime(std::localtime(&t));

    // asctime puts a newline at the end; remove it
    ts.pop_back();
    _data->setTitle(label + " (" + ts + ")");

    connect(this, SIGNAL(doneReceiving(void)), _data.get(), SLOT(setDoneReceiving(void)));
    connect(this, &Execution::doneReceiving, [this]() {
            _is_done = true;
            std::cerr << "execution " << this << " done receiving\n";
        });

    std::cerr << "Execution::start on " << this << "\n";

    builder = new TreeBuilder(this);

    connect(builder, &TreeBuilder::addedNode, this, &Execution::newNode);
    connect(builder, &TreeBuilder::addedRoot, this, &Execution::newRoot);

    builder->start();

    emit titleKnown();
}

const std::string* Execution::getNogood(const Node& node) const {
    auto entry = getEntry(node);
    if (!entry) return nullptr;
    auto nogood = _data->getNogoods().find(entry->full_sid);
    if (nogood == _data->getNogoods().end()) return nullptr;
    return &nogood->second;
}

const std::string* Execution::getInfo(const Node& node) const {
    auto entry = getEntry(node);
    if (!entry) return nullptr;
    auto info = _data->sid2info.find(entry->s_node_id);
    if (info == _data->sid2info.end()) return nullptr;
    return info->second;
}

void Execution::handleNewNode(message::Node& node) {
    _data->handleNodeCallback(node);
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
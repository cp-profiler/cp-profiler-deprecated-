#include "execution.hh"
#include "data.hh"
#include "treebuilder.hh"
#include "cpprofiler/utils/tree_utils.hh"

#include <regex>

using std::string;

Execution::Execution()
    : m_NodeTree{new NodeTree},
      m_Data{new Data()},
      m_Builder{new TreeBuilder(this)},
      execution_id(0) {}

Execution::Execution(std::unique_ptr<NodeTree> nt, std::unique_ptr<Data> data)
    : m_NodeTree{std::move(nt)},  m_Data{std::move(data)}, execution_id(0)  {}

Execution::~Execution() {}

Data& Execution::getData() const {
    return *m_Data.get();
}

void Execution::start(std::string label, bool isRestarts) {

    _is_restarts = isRestarts;

    std::time_t t = std::time(nullptr);
    string ts = std::asctime(std::localtime(&t));

    // asctime puts a newline at the end; remove it
    ts.pop_back();
    setTitle(label + " (" + ts + ")");

    connect(this, SIGNAL(doneReceiving(void)), m_Data.get(), SLOT(setDoneReceiving(void)));

    std::cerr << "Execution::start on " << this << "\n";

    connect(m_Builder.get(), &TreeBuilder::addedNode, this, &Execution::newNode);
    connect(m_Builder.get(), &TreeBuilder::addedRoot, this, &Execution::newRoot);

    m_Builder->start();

    emit titleKnown();
}

const std::string* Execution::getNogood(const Node& node) const {
    auto entry = getEntry(node);
    if (!entry) return nullptr;
    auto nogood = m_Data->getNogoods().find(entry->s_node_id);
    if (nogood == m_Data->getNogoods().end()) return nullptr;
    return &nogood->second;
}

const std::string* Execution::getInfo(const Node& node) const {
    auto entry = getEntry(node);
    if (!entry) return nullptr;
    auto info = m_Data->sid2info.find(entry->full_sid);
    if (info == m_Data->sid2info.end()) return nullptr;
    return info->second;
}

void Execution::handleNewNode(message::Node& node) {
    m_Data->handleNodeCallback(node);
}

const std::unordered_map<int, string>& Execution::getNogoods() const {
  return m_Data->getNogoods();
}
std::unordered_map<int64_t, string*>& Execution::getInfo(void) const {
  return m_Data->getInfo();
}
DbEntry* Execution::getEntry(int gid) const { return m_Data->getEntry(gid); }
unsigned int Execution::getGidBySid(int sid) { return m_Data->getGidBySid(sid); }
std::string Execution::getLabel(int gid) const { return m_Data->getLabel(gid); }
unsigned long long Execution::getTotalTime() { return m_Data->getTotalTime(); }

void Execution::compareDomains() {

  const auto& na = m_NodeTree->getNA();

  std::vector<VisualNode*> target_nodes;

  for (auto i=0; i < na.size(); ++i) {
    auto node = na[i];
    if (node->isHighlighted()) {
      target_nodes.push_back(node);
    }
  }

  if (target_nodes.size() < 2) return;

  auto l_node = target_nodes[0];
  auto r_node = target_nodes[1];

  qDebug() << utils::compareDomains(*this, *l_node, *r_node).c_str();

}

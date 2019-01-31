#include "execution.hh"
#include "data.hh"
#include "treebuilder.hh"
#include "nodecursor.hh"
#include "globalhelper.hh"
#include "nodevisitor.hh"
#include "cpprofiler/utils/tree_utils.hh"

#include <thread>

using std::string;
namespace Profiling {
class Message;
}

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

static void deleteNode(Execution& ex, Node* n) {

  auto& na = ex.nodeTree().getNA();

  auto parent = n->getParent(na);
  if (!parent) return;
  parent->removeChild(n->getIndex(na));
  parent->dirtyUp(na);
}

static void deleteSkippedNodes(Execution& ex) {

  auto& na = ex.nodeTree().getNA();

  for (auto i = 0; i < na.size(); ++i) {
    auto node = na[i];
    if (node->getStatus() == SKIPPED) {
      deleteNode(ex, node);
    }
  }
}

static void deleteWhiteNodes(Execution& ex) {
  // TODO(maxim): might have to reset 'current node' if
  //              is is no longer visible
  // TODO(maxim): fix this crashing pixel tree

  // IMPORTANT(maxim): currently I leave 'holes' in *na* that
  //                   can be examined and point to nodes not visualised

  // 1. swap deleted nodes in *na* with the last in *na*
  // and remap array ids.

  auto& na = ex.nodeTree().getNA();

  for (auto i = 0; i < na.size(); ++i) {
    auto node = na[i];
    if (node->getStatus() == UNDETERMINED) {
      deleteNode(ex, node);
    }
  }
}

static void printSearchLog(Execution& ex) {
  QString path;

  if (GlobalParser::isSet(GlobalParser::save_log)) {
    path = "search.log";
  } else {
    path = QFileDialog::getSaveFileName(nullptr, "Save File", "");
  }

  deleteSkippedNodes(ex);
  deleteWhiteNodes(ex);

  QFile file(path);

  if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
    qDebug() << "could not open the file: " << path;
    return;
  }

  QTextStream out(&file);

  auto& nt = ex.nodeTree();
  auto root = nt.getRoot();
  SearchLogCursor slc(root, out, nt.getNA(), ex);
  PreorderNodeVisitor<SearchLogCursor>(slc).run();

  qDebug() << "SEARCH LOG READY\n";


}



void Execution::begin(std::string label, bool isRestarts) {

    _is_restarts = isRestarts;

    setTitle(label);

    // qDebug() << "Execution::start";
    connect(this, &Execution::doneReceiving, this, [this]() {
      qDebug() << "setDoneReceiving";
      m_Data->setDoneReceiving();
    });

    connect(m_Builder.get(), &TreeBuilder::addedNode, this, &Execution::newNode);
    connect(m_Builder.get(), &TreeBuilder::addedRoot, this, &Execution::newRoot);

    qDebug() << "m_Builder.get(): " << m_Builder.get() << "\n";
    connect(m_Builder.get(), &TreeBuilder::doneBuilding, this, [this]() {

      if (GlobalParser::isSet(GlobalParser::save_log)) {
        printSearchLog(*this);
      }

      finished = true;

      emit doneBuilding();
    }, Qt::QueuedConnection);

    m_Data->initReceiving();
    m_Builder->start();

    emit titleKnown();
}

const NogoodViews* Execution::getNogood(const Node& node) const {
    auto entry = getEntry(node);
    if (!entry) return nullptr;
    auto nogood = m_Data->getNogoods().find(entry->nodeUID);
    if (nogood == m_Data->getNogoods().end()) return nullptr;
    return &nogood->second;
}

NodeUID Execution::getParentUID(const NodeUID uid) const {
  DbEntry* entry = getEntry(getGidByUID(uid));
  return entry->parentUID;
}

const std::string* Execution::getInfo(const Node& node) const {
    auto entry = getEntry(node);
    if (!entry) return nullptr;
    return getInfo(entry->nodeUID);
}

const int* Execution::getObjective(const Node& node) const {
  auto entry = getEntry(node);
  if (!entry) return nullptr;
  return m_Data->getObjective(entry->nodeUID);
}

const std::string* Execution::getInfo(NodeUID uid) const {
  auto info = m_Data->uid2info.find(uid);
  if (info == m_Data->uid2info.end()) return nullptr;
  return info->second.get();
}

Statistics& Execution::getStatistics() {
        return m_NodeTree->getStatistics();
}

void Execution::handleNewNode(const cpprofiler::Message& msg) {
    m_Data->handleNodeCallback(msg);
}

const Uid2Nogood& Execution::getNogoods() const {
  return m_Data->getNogoods();
}

static std::string empty_string;
const std::string& Execution::getNogoodByUID(NodeUID uid, bool renamed, bool simplified) const {
  const auto& ng_map = m_Data->getNogoods();
  auto maybe_nogood = ng_map.find(uid);
  if (maybe_nogood != ng_map.end()){
    const NogoodViews& ng = maybe_nogood->second;

    if(renamed && (ng.renamed != "")) {
      return simplified ? ng.simplified : ng.renamed;
    } else {
      return ng.original;
    }
  }

  return empty_string;
}

DbEntry* Execution::getEntry(int gid) const { return m_Data->getEntry(gid); }

DbEntry* Execution::getEntry(const Node& node) const {
    auto gid = node.getIndex(m_NodeTree->getNA());
    return getEntry(gid);
}

int32_t Execution::getGidByUID(NodeUID uid) const { return m_Data->getGidByUID(uid); }

std::string Execution::getLabel(int gid, bool rename) const {
  std::string origLabel = m_Data->getLabel(gid);
  if(rename) {
    const auto* nm = m_Data->getNameMap();
    return nm ? nm->replaceNames(origLabel, true) : origLabel;
  }
  return origLabel;
}

std::string Execution::getLabel(const VisualNode& node, bool rename) const {
  auto gid = node.getIndex(m_NodeTree->getNA());
  return getLabel(gid, rename);
}

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

const NameMap* Execution::getNameMap() const { return m_Data->getNameMap(); }
void Execution::setNameMap(NameMap* names) {
  m_Data->setNameMap(names);
}


#ifdef MAXIM_DEBUG
void Execution::setLabel(const VisualNode& n, const std::string& str) {
  auto gid = n.getIndex(m_NodeTree->getNA());
  m_Data->setLabel(gid, str);
}
#endif

QMutex& Execution::getTreeMutex() { return m_NodeTree->getTreeMutex(); }
QMutex& Execution::getLayoutMutex() { return m_NodeTree->getLayoutMutex(); }

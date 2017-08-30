#include "execution.hh"
#include "data.hh"
#include "treebuilder.hh"
#include "nodecursor.hh"
#include "globalhelper.hh"
#include "nodevisitor.hh"
#include "cpprofiler/utils/tree_utils.hh"

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

static void printSearchLog(Execution& ex) {

  QString path;

  if (GlobalParser::isSet(GlobalParser::save_log)) {
    path = "search.log";
  } else {
    path = QFileDialog::getSaveFileName(nullptr, "Save File", "");
  }

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

  std::cout << "SEARCH LOG READY" << std::endl;


}

static void deleteNode(Execution& ex, Node* n) {

  auto& na = ex.nodeTree().getNA();

  auto parent = n->getParent(na);
  if (!parent) return;
  parent->removeChild(n->getIndex(na), na);
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


void Execution::start(std::string label, bool isRestarts) {

    _is_restarts = isRestarts;

    std::time_t t = std::time(nullptr);
    string ts = std::asctime(std::localtime(&t));

    // asctime puts a newline at the end; remove it
    ts.pop_back();
    setTitle(label + " (" + ts + ")");

    connect(this, SIGNAL(doneReceiving(void)), m_Data.get(), SLOT(setDoneReceiving(void)));

    connect(m_Builder.get(), &TreeBuilder::addedNode, this, &Execution::newNode);
    connect(m_Builder.get(), &TreeBuilder::addedRoot, this, &Execution::newRoot);

    connect(m_Builder.get(), &TreeBuilder::doneBuilding, [this]() {

      if (GlobalParser::isSet(GlobalParser::save_log)) {
        printSearchLog(*this);
      }

      finished = true;
      emit doneBuilding();
    });

    m_Builder->start();

    emit titleKnown();
}

const NogoodViews* Execution::getNogood(const Node& node) const {
    auto entry = getEntry(node);
    if (!entry) return nullptr;
    auto nogood = m_Data->getNogoods().find(entry->full_sid);
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

Statistics& Execution::getStatistics() {
        return m_NodeTree->getStatistics();
}

void Execution::handleNewNode(const cpprofiler::Message& msg) {
    m_Data->handleNodeCallback(msg);
}

const Sid2Nogood& Execution::getNogoods() const {
  return m_Data->getNogoods();
}

static std::string empty_string;
const std::string& Execution::getNogoodBySid(int64_t sid, bool renamed, bool simplified) const {
  const auto& ng_map = m_Data->getNogoods();
  auto maybe_nogood = ng_map.find(sid);
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

std::unordered_map<int64_t, string*>& Execution::getInfo(void) const {
  return m_Data->getInfo();
}

DbEntry* Execution::getEntry(int gid) const { return m_Data->getEntry(gid); }

DbEntry* Execution::getEntry(const Node& node) const {
    auto gid = node.getIndex(m_NodeTree->getNA());
    return getEntry(gid);
}

unsigned Execution::getGidBySid(int64_t sid) { return m_Data->getGidBySid(sid); }
std::string Execution::getLabel(int gid) const {
  std::string origLabel = m_Data->getLabel(gid);
  const auto* nm = m_Data->getNameMap();
  return nm ? nm->replaceNames(origLabel) : origLabel;
}




std::string Execution::getLabel(const VisualNode& node) const {
  auto gid = node.getIndex(m_NodeTree->getNA());
  return getLabel(gid);
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

QMutex& Execution::getMutex() { return m_NodeTree->getMutex(); }
QMutex& Execution::getLayoutMutex() { return m_NodeTree->getLayoutMutex(); }

#include "profiler-conductor.hh"
#include "profiler-tcp-server.hh"

#include "gistmainwindow.h"
#include "cmp_tree_dialog.hh"
#include "data.hh"

#include "globalhelper.hh"
#include "libs/perf_helper.hh"
#include "treecanvas.hh"
#include "receiverthread.hh"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

#include "message.pb.hh"

#include "ml-stats.hh"
#include "webscript.hh"

#include "tree_utils.hh"

#include <fstream>

#include <QPushButton>
#include <QVBoxLayout>

#include <thread>
#include <chrono>

#include <QDebug>

#include "subtree_comparison.hpp";

bool readDelimitedFrom(google::protobuf::io::ZeroCopyInputStream* rawInput,
                       google::protobuf::MessageLite* message) {
  // We create a new coded stream for each message.  Don't worry, this is fast,
  // and it makes sure the 64MB total size limit is imposed per-message rather
  // than on the whole stream.  (See the CodedInputStream interface for more
  // info on this limit.)
  google::protobuf::io::CodedInputStream input(rawInput);

  // Read the size.
  uint32_t size;
  if (!input.ReadVarint32(&size)) return false;

  // Tell the stream not to read beyond that size.
  google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);

  // Parse the message.
  if (!message->ParseFromCodedStream(&input)) return false;
  if (!input.ConsumedEntireMessage()) return false;

  // Release the limit.
  input.PopLimit(limit);

  return true;
}

using google::protobuf::io::IstreamInputStream;

Execution* loadSaved(Execution* e, std::string path) {
  std::ifstream inputFile(path, std::ios::in | std::ios::binary);
  IstreamInputStream raw_input(&inputFile);
  while (true) {
    message::Node msg;
    bool ok = readDelimitedFrom(&raw_input, &msg);
    if (!ok) break;
    switch (msg.type()) {
      case message::Node::NODE: {
          e->handleNewNode(msg);
          break;
      }
      case message::Node::START: {
        bool is_restarts = (msg.restart_id() == 0);
        e->start("loaded from " + path, is_restarts);
        if (msg.has_info()) {
            e->setVariableListString(msg.info());
        }
        break;
      }
    }
  }
  e->doneReceiving();
  return e;
}

ProfilerConductor::ProfilerConductor() : QMainWindow() {

  executionList.setSelectionMode(QAbstractItemView::MultiSelection);
  compareWithLabelsCB.setText("with labels");

  auto centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  auto gistButton = new QPushButton("show tree");
  connect(gistButton, SIGNAL(clicked()), this,
          SLOT(gistButtonClicked()));

  auto compareButton = new QPushButton("compare trees");
  connect(compareButton, SIGNAL(clicked()), this,
          SLOT(compareButtonClicked()));

  auto compareSubtrees = new QPushButton("compare subtrees");
  connect(compareSubtrees, &QPushButton::clicked,
          this, &ProfilerConductor::compareSubtrees);

  auto gatherStatisticsButton = new QPushButton("gather statistics");
  connect(gatherStatisticsButton, SIGNAL(clicked()), this,
          SLOT(gatherStatisticsClicked()));

  auto webscriptButton = new QPushButton("webscript view");
  connect(webscriptButton, SIGNAL(clicked()), this,
          SLOT(webscriptClicked()));

  auto saveExecutionButton = new QPushButton("save execution");
  connect(saveExecutionButton, SIGNAL(clicked()), this,
          SLOT(saveExecutionClicked()));

  auto loadExecutionButton = new QPushButton("load execution");
  connect(loadExecutionButton, SIGNAL(clicked()), this,
          SLOT(loadExecutionClicked()));

  auto deleteExecutionButton = new QPushButton("delete execution");
  connect(deleteExecutionButton, SIGNAL(clicked()), this,
          SLOT(deleteExecutionClicked()));

  auto debugExecutionButton = new QPushButton("debug execution");
  connect(debugExecutionButton, SIGNAL(clicked()), this,
          SLOT(createDebugExecution()));

  auto layout = new QGridLayout();

  layout->addWidget(&executionList,         0, 0, 1, 2);
  layout->addWidget(gistButton,             1, 0, 1, 2);

  layout->addWidget(compareButton,          2, 0, 1, 1);
  layout->addWidget(&compareWithLabelsCB,   2, 1, 1, 1);

  layout->addWidget(compareSubtrees,        3, 0, 1, 2);
  layout->addWidget(gatherStatisticsButton, 4, 0, 1, 2);
  layout->addWidget(webscriptButton,        5, 0, 1, 2);
  layout->addWidget(saveExecutionButton,    6, 0, 1, 2);
  layout->addWidget(loadExecutionButton,    7, 0, 1, 2);
  layout->addWidget(deleteExecutionButton,  8, 0, 1, 2);

#ifdef MAXIM_DEBUG
  layout->addWidget(debugExecutionButton,   9, 0, 1, 2);
#endif

  centralWidget->setLayout(layout);

  // Listen for new executions.
  listener.reset(new ProfilerTcpServer([this](qintptr socketDescriptor) {
    auto execution = new Execution();

    /// TODO(maxim): receiver should be destroyed when done
    auto receiver = new ReceiverThread(socketDescriptor, execution, this);

    addExecution(*execution);

    receiver->start();
  }));

  listener->listen(QHostAddress::Any, 6565);
}

ProfilerConductor::~ProfilerConductor() = default;

class ExecutionListItem : public QListWidgetItem {
 public:
  ExecutionListItem(Execution& execution, QListWidget* parent)
      : QListWidgetItem{parent, QListWidgetItem::Type},
        execution_{execution}
    {}

  Execution& execution_;
};


void ProfilerConductor::addExecution(Execution& e) {
  auto newItem = new ExecutionListItem(e, &executionList);

  std::string title = (e.getTitle() == "") ? "some execution" : e.getTitle();
  newItem->setText(title.c_str());
  newItem->setSelected(true);
  executionList.addItem(newItem);
  executions << &e;

  executionInfoHash.insert(&e, new ExecutionInfo);

  /// updates titles when a new one becomes known
  connect(&e, SIGNAL(titleKnown()), this, SLOT(updateTitles()));

  show();
}

void ProfilerConductor::displayExecution(Execution& ex, QString&& title) {
  auto gistMW = executionInfoHash[&ex]->gistWindow;
  if (gistMW == nullptr) {
    gistMW = createGist(ex, title);
  }
  gistMW->show();
  gistMW->activateWindow();
  connect(gistMW->getCanvas(), &TreeCanvas::announceSelectNode,
          this, [this, &ex](int gid){
            this->tellVisualisationsSelectNode(&ex, gid);
          });
}

void ProfilerConductor::updateTitles(void) {
  for (int i = 0; i < executions.size(); i++) {
    executionList.item(i)
        ->setText(QString::fromStdString(executions[i]->getTitle()));
  }
}

GistMainWindow* ProfilerConductor::createGist(Execution& e, QString title) {
  auto gist = new GistMainWindow{e, this};
  gist->changeTitle(title);
  executionInfoHash[&e]->gistWindow = gist;
  return gist;
}

void ProfilerConductor::gistButtonClicked() {
  for (auto* l_item : executionList.selectedItems()) {
    auto item = static_cast<ExecutionListItem*>(l_item);
    Execution& ex = item->execution_;
    
    displayExecution(ex, item->text());
  }
}

void ProfilerConductor::compareButtonClicked() {
  QList<QListWidgetItem*> selected = executionList.selectedItems();
  if (selected.size() != 2) return;

  auto ex1 = &static_cast<ExecutionListItem*>(selected[0])->execution_;
  auto ex2 = &static_cast<ExecutionListItem*>(selected[1])->execution_;

  auto gmw1 = executionInfoHash[ex1]->gistWindow;
  auto gmw2 = executionInfoHash[ex2]->gistWindow;
  if (gmw1 == nullptr || gmw2 == nullptr) return;

  const bool withLabels = compareWithLabelsCB.isChecked();

  /// NOTE(maxim): the new window will delete itself when closed
  new CmpTreeDialog(this, new Execution{}, withLabels, *ex1, *ex2);
}


void ProfilerConductor::compareSubtrees() {

  QList<QListWidgetItem*> selected = executionList.selectedItems();
  if (selected.size() != 2) return;

  const auto* ex1 = &static_cast<ExecutionListItem*>(selected[0])->execution_;
  const auto* ex2 = &static_cast<ExecutionListItem*>(selected[1])->execution_;

  auto gmw1 = executionInfoHash[ex1]->gistWindow;
  auto gmw2 = executionInfoHash[ex2]->gistWindow;

  if (gmw1 == nullptr || gmw2 == nullptr) return;

  auto* e = new Execution{};
  addExecution(*e);

  subtree_comparison::compareExecutions(*e, *ex1, *ex2);

  displayExecution(*e, "combined tree");
}

class StatsHelper {
  QString filename;
  StatsHelper(QString filename_) : filename(filename_) {}
};

using google::protobuf::io::OstreamOutputStream;

bool writeDelimitedTo(const google::protobuf::MessageLite& message,
                      google::protobuf::io::ZeroCopyOutputStream* rawOutput) {
  // We create a new coded stream for each message.  Don't worry, this is fast.
  google::protobuf::io::CodedOutputStream output(rawOutput);

  // Write the size.
  const int size = message.ByteSize();
  output.WriteVarint32(size);

  uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
  if (buffer != nullptr) {
    // Optimization:  The message fits in one buffer, so use the faster
    // direct-to-array serialization path.
    message.SerializeWithCachedSizesToArray(buffer);
  } else {
    // Slightly-slower path when the message is multiple buffers.
    message.SerializeWithCachedSizes(&output);
    if (output.HadError()) return false;
  }

  return true;
}

void ProfilerConductor::gatherStatisticsClicked() {
  QList<QListWidgetItem*> selected = executionList.selectedItems();
  for (int i = 0; i < selected.size(); i++) {
    ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);

    GistMainWindow* g = executionInfoHash[&item->execution_]->gistWindow;

    if (g == nullptr) {
      g = new GistMainWindow(item->execution_, this);
      executionInfoHash[&item->execution_]->gistWindow = g;
    }
    //        g->hide();

    // This is a big mess.  The user must wait for the tree to be
    // built before they select their file.

    QString filename = QFileDialog::getSaveFileName(this, "Save statistics",
                                                    QDir::currentPath());
    // Very inelegant
    g->setStatsFilename(filename);

    g->gatherStatistics();

  }
}

void ProfilerConductor::webscriptClicked() {
    QList<QListWidgetItem*> selected = executionList.selectedItems();
    for (int i = 0; i < selected.size(); i++) {
        ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);
        Execution& execution = item->execution_;

        const char* paths[] = { "webscripts/sunburst.html", "webscripts/icicle.html", "webscripts/variables.html" };
        const char* ids[] = { "sunburst", "icicle", "variables" };
        std::stringstream ss;
        ::collectMLStats(execution.nodeTree().getRoot(), execution.nodeTree().getNA(), &execution, ss);

        for (int i = 0 ; i < 3 ; i++) {
            if (getWebscriptView(&execution, ids[i]) == NULL) {
                QDialog* dialog = new QDialog(this);
                WebscriptView* web = new WebscriptView(dialog, paths[i], &execution, ss.str());
                registerWebscriptView(&execution, ids[i], web);
            
                QHBoxLayout* layout = new QHBoxLayout;
                layout->addWidget(web);
                dialog->setLayout(layout);

                connect(web, &WebscriptView::announceSelectNode,
                        this, [this, &execution](int gid){this->tellVisualisationsSelectNode(&execution, gid);});
                connect(web, &WebscriptView::announceSelectManyNodes,
                        this, [this, &execution](QList<QVariant> gids){
                            this->tellVisualisationsSelectManyNodes(&execution, gids);
                        });
            
                dialog->show();
            }

            // We get the parent widget here because we want to show()
            // the containing QDialog, not the inner WebEngineView.
            getWebscriptView(&execution, ids[i])->parentWidget()->show();
        }
    }
}

void ProfilerConductor::tellVisualisationsSelectNode(Execution* execution, int gid) {
    WebscriptView* p;
    p = executionInfoHash[execution]->sunburstView;  if (p) p->select(gid);
    p = executionInfoHash[execution]->icicleView;    if (p) p->select(gid);
    p = executionInfoHash[execution]->variablesView; if (p) p->select(gid);
    GistMainWindow* g;
    g = executionInfoHash[execution]->gistWindow;    if (g) g->selectNode(gid);
}

void ProfilerConductor::tellVisualisationsSelectManyNodes(Execution* execution, QList<QVariant> gids) {
    WebscriptView* p;
    p = executionInfoHash[execution]->sunburstView;  if (p) p->selectMany(gids);
    p = executionInfoHash[execution]->icicleView;    if (p) p->selectMany(gids);
    p = executionInfoHash[execution]->variablesView; if (p) p->selectMany(gids);
    GistMainWindow* g;
    g = executionInfoHash[execution]->gistWindow;    if (g) g->selectManyNodes(gids);
}

void ProfilerConductor::saveExecutionClicked() {
  QList<QListWidgetItem*> selected = executionList.selectedItems();
  if (selected.size() != 1) return;
  auto item = static_cast<ExecutionListItem*>(selected[0]);

  QString filename =
      QFileDialog::getSaveFileName(this, "Save execution", QDir::currentPath());
  std::ofstream outputFile(filename.toStdString(),
                           std::ios::out | std::ios::binary);
  OstreamOutputStream raw_output(&outputFile);
  auto& data = item->execution_.getData();

  std::vector<unsigned int> keys;
  keys.reserve(data.gid2entry.size());

  for (const auto& pair : data.gid2entry) {
    keys.push_back(pair.first);
  }
  sort(keys.begin(), keys.end());

  message::Node start_node;
  start_node.set_type(message::Node::START);

  if (item->execution_.isRestarts()) {
    start_node.set_restart_id(0);
  } else {
    start_node.set_restart_id(-1);
  }

  start_node.set_info(item->execution_.getVariableListString());
  writeDelimitedTo(start_node, &raw_output);

  for (auto& key : keys) {
    DbEntry* entry = data.gid2entry[key];
    message::Node node;
    node.set_type(message::Node::NODE);
    node.set_sid(entry->s_node_id);
    node.set_pid(entry->parent_sid);
    node.set_alt(entry->alt);
    node.set_kids(entry->numberOfKids);
    node.set_status(static_cast<message::Node::NodeStatus>(entry->status));
    //            node.set_restart_id
    node.set_time(entry->time_stamp);
    node.set_thread_id(entry->thread_id);
    node.set_label(entry->label);
    node.set_domain_size(entry->domain);
    //            node.set_solution(entry->);
    auto ngit = data.getNogoods().find(entry->s_node_id);
    if (ngit != data.getNogoods().end()) node.set_nogood(ngit->second);
    auto infoit = data.sid2info.find(entry->s_node_id);
    if (infoit != data.sid2info.end()) node.set_info(*infoit->second);
    writeDelimitedTo(node, &raw_output);
  }
}

void ProfilerConductor::loadExecutionClicked() {
  QString filename =
      QFileDialog::getOpenFileName(this, "Load execution", QDir::currentPath());
  if (!filename.isNull())
      loadExecution(filename.toStdString());
}



void ProfilerConductor::loadExecution(std::string filename) {
  auto e = new Execution();
  addExecution(*e);
  loadSaved(e, filename);
}

void ProfilerConductor::deleteExecutionClicked() {
  QList<QListWidgetItem*> selected = executionList.selectedItems();
  for (int i = 0; i < selected.size(); i++) {
    auto item = static_cast<ExecutionListItem*>(selected[i]);
    executions.removeOne(&item->execution_);
    delete &item->execution_;
    if (executionInfoHash[&item->execution_]->gistWindow != NULL) {
        executionInfoHash[&item->execution_]->gistWindow->stopReceiver();
        delete executionInfoHash[&item->execution_]->gistWindow;
    }
    delete item;
  }
}

WebscriptView*
ProfilerConductor::getWebscriptView(Execution* execution, std::string id) {
    if      (id == "sunburst")  return executionInfoHash[execution]->sunburstView;
    else if (id == "icicle")    return executionInfoHash[execution]->icicleView;
    else if (id == "variables") return executionInfoHash[execution]->variablesView;
    return NULL;
}

void ProfilerConductor::registerWebscriptView(Execution* execution, std::string id, WebscriptView* webView) {
    if      (id == "sunburst")  executionInfoHash[execution]->sunburstView = webView;
    else if (id == "icicle")    executionInfoHash[execution]->icicleView = webView;
    else if (id == "variables") executionInfoHash[execution]->variablesView = webView;
}

void ProfilerConductor::createDebugExecution() {
  qDebug() << "createDebugExecution";
  auto e = new Execution{};
  addExecution(*e);

  // auto& na = e->getNA();

  // na.allocateRoot();

  // auto root = na[0];

  // root->setStatus(BRANCH);
  // root->setNumberOfChildren(2, na);

  // auto kid1 = root->getChild(na, 0);
  // kid1->setStatus(FAILED);

  // auto kid2 = root->getChild(na, 0);
  // kid2->setStatus(BRANCH);
  // kid2->setNumberOfChildren(2, na);

  // kid1->dirtyUp(na);
  // kid2->dirtyUp(na);

}

using namespace std;

void ProfilerConductor::createExecution(unique_ptr<NodeTree> nt,
                                        unique_ptr<Data> data) {

  auto e = new Execution(std::move(nt), std::move(data));
  addExecution(*e);

  auto gist = createGist(*e, e->getTitle().c_str());
}
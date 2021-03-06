#include "profiler-conductor.hh"
#include "profiler-tcp-server.hh"

#include "gistmainwindow.h"
#include "cmp_tree_dialog.hh"
#include "data.hh"

#include "globalhelper.hh"
#include "libs/perf_helper.hh"
#include "treecanvas.hh"
#include "receiverthread.hh"

#include "ml-stats.hh"
//#include "webscript.hh"

#include "cpprofiler/utils/tree_utils.hh"
#include "cpprofiler/utils/utils.hh"

#include <fstream>
#include <QPushButton>
#include <QVBoxLayout>

#include <thread>
#include <chrono>

#include <QDebug>

#include "subtree_comparison.hpp"

// Execution* loadSaved(Execution* e, std::string path) {
//   std::ifstream inputFile(path, std::ios::in | std::ios::binary);
//   IstreamInputStream raw_input(&inputFile);
//   while (true) {
//     message::Node msg;
//     bool ok = readDelimitedFrom(&raw_input, &msg);
//     if (!ok) break;
//     switch (msg.type()) {
//       case message::Node::NODE: {
//           e->handleNewNode(msg);
//           break;
//       }
//       case message::Node::START: {
//         bool is_restarts = (msg.restart_id() == 0);
//         e->start("loaded from " + path, is_restarts);
//         if (msg.has_info()) {
//             e->setVariableListString(msg.info());
//         }
//         break;
//       }
//     }
//   }
//   e->doneReceiving();
//   return e;
// }

ProfilerConductor::ProfilerConductor() : QMainWindow(), listen_port(6565) {
  executionTreeView.setModel(&executionTreeModel);
  executionTreeView.setSelectionMode(QAbstractItemView::MultiSelection);

  QStringList header{"Executions:"};
  executionTreeModel.setHorizontalHeaderLabels(header);
  compareWithLabelsCB.setText("with labels");
  compareWithLabelsCB.setEnabled(false);
  compareWithLabelsCB.setChecked(true);

  auto centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  auto gistButton = new QPushButton("show tree");
  gistButton->setEnabled(false);
  connect(gistButton, SIGNAL(clicked()), this,
          SLOT(gistButtonClicked()));

  auto compareButton = new QPushButton("compare trees");
  compareButton->setEnabled(false);
  connect(compareButton, SIGNAL(clicked()), this,
          SLOT(compareButtonClicked()));

  auto compareSubtrees = new QPushButton("compare subtrees");
  compareSubtrees->setEnabled(false);
  connect(compareSubtrees, &QPushButton::clicked,
          this, &ProfilerConductor::compareSubtrees);

  auto gatherStatisticsButton = new QPushButton("gather statistics");
  gatherStatisticsButton->setEnabled(false);
  connect(gatherStatisticsButton, SIGNAL(clicked()), this,
          SLOT(gatherStatisticsClicked()));

  // auto webscriptButton = new QPushButton("webscript view");
  // webscriptButton->setEnabled(false);
  // connect(webscriptButton, SIGNAL(clicked()), this,
  //         SLOT(webscriptClicked()));

  auto saveExecutionButton = new QPushButton("save execution");
  saveExecutionButton->setEnabled(false);
  connect(saveExecutionButton, SIGNAL(clicked()), this,
          SLOT(saveExecutionClicked()));

  auto loadExecutionButton = new QPushButton("load execution");
  connect(loadExecutionButton, SIGNAL(clicked()), this,
          SLOT(loadExecutionClicked()));

  auto deleteExecutionButton = new QPushButton("delete execution");
  deleteExecutionButton->setEnabled(false);
  connect(deleteExecutionButton, SIGNAL(clicked()), this,
          SLOT(deleteExecutionClicked()));

  auto debugExecutionButton = new QPushButton("debug execution");
  connect(debugExecutionButton, SIGNAL(clicked()), this,
          SLOT(createDebugExecution()));

  auto cloneExecutionButton = new QPushButton("clone execution");
  connect(cloneExecutionButton, &QPushButton::clicked, this,
          &ProfilerConductor::cloneExecution);

  connect(executionTreeView.selectionModel(), &QItemSelectionModel::selectionChanged,
          [=](const QItemSelection& selected, const QItemSelection& deselected) {
      auto selectionModel = executionTreeView.selectionModel();
      for(auto ind : selected.indexes()) {
        QStandardItem* item = executionTreeModel.itemFromIndex(ind);
        if(item != executionTreeModel.invisibleRootItem()) {
          int rows = item->rowCount();
          for(int i=0; i<rows; i++) {
            QStandardItem* child = item->child(i, 0);
            selectionModel->select(executionTreeModel.indexFromItem(child),
                                   QItemSelectionModel::Select);
          }
        }
      }
      for(auto ind : deselected.indexes()) {
        QStandardItem* item = executionTreeModel.itemFromIndex(ind);
        if(item != executionTreeModel.invisibleRootItem()) {
          selectionModel->select(executionTreeModel.indexFromItem(item->parent()),
                                 QItemSelectionModel::Deselect);
          int rows = item->rowCount();
          for(int i=0; i<rows; i++) {
            QStandardItem* child = item->child(i, 0);
            selectionModel->select(executionTreeModel.indexFromItem(child),
                                   QItemSelectionModel::Deselect);
          }
        }
      }

      int nselected = getSelectedExecutions().size();
      gistButton->setEnabled            (nselected > 0);
      compareButton->setEnabled         (nselected == 2);
      compareSubtrees->setEnabled       (nselected == 2);
      compareWithLabelsCB.setEnabled    (nselected == 2);
      gatherStatisticsButton->setEnabled(nselected > 0);
      // webscriptButton->setEnabled       (nselected > 0);
      saveExecutionButton->setEnabled   (nselected == 1);
      loadExecutionButton->setEnabled   (true);
      deleteExecutionButton->setEnabled (nselected > 0);
      debugExecutionButton->setEnabled  (true);
      cloneExecutionButton->setEnabled  (nselected == 1);
  });

  auto layout = new QGridLayout();

  layout->addWidget(&executionTreeView,     0, 0, 1, 2);
  layout->addWidget(gistButton,             1, 0, 1, 2);

  layout->addWidget(compareButton,          2, 0, 1, 1);
  layout->addWidget(&compareWithLabelsCB,   2, 1, 1, 1);

  layout->addWidget(compareSubtrees,        3, 0, 1, 2);
  layout->addWidget(gatherStatisticsButton, 4, 0, 1, 2);
  // layout->addWidget(webscriptButton,        5, 0, 1, 2);
  layout->addWidget(saveExecutionButton,    6, 0, 1, 2);
  layout->addWidget(loadExecutionButton,    7, 0, 1, 2);
  layout->addWidget(cloneExecutionButton,   8, 0, 1, 2);
  layout->addWidget(deleteExecutionButton,  9, 0, 1, 2);

#ifdef MAXIM_DEBUG
  layout->addWidget(debugExecutionButton,   10, 0, 1, 2);
#endif

  centralWidget->setLayout(layout);

  // Listen for new executions.
  listener.reset(new ProfilerTcpServer([this](qintptr socketDescriptor) {
    // auto execution = new Execution();

    if (!latest_execution) {
      latest_execution = new Execution();
    }

    /// TODO(maxim): receiver should be destroyed when done?
    auto receiver = new ReceiverThread(socketDescriptor, latest_execution, this);

    connect(receiver, &ReceiverThread::executionIdReady,
      this, &ProfilerConductor::executionIdReady);

    connect(receiver, &ReceiverThread::executionStarted,
      this, &ProfilerConductor::executionStarted);

    connect(receiver, &ReceiverThread::doneReceiving, [this]() {
      latest_execution = nullptr;
    });

    receiver->start();
  }));

  if(!listener->listen(QHostAddress::Any, listen_port)) {
    listener->listen(QHostAddress::Any, 0); // Try any port
    listen_port = listener->serverPort();
  }

  setWindowTitle("CPProfiler");
  std::cerr << "READY TO LISTEN ON: " << listen_port << " \n";
}

ProfilerConductor::~ProfilerConductor() {
  for (auto ex : executions) {
    delete ex;
  }
}

int ProfilerConductor::getListenPort() {
  return static_cast<int>(listen_port);
}

int ProfilerConductor::getNextExecId(const std::string& group_name,
                                     const std::string& execution_name,
                                     const NameMap& nameMap) {
   // walk until we find an unused execution id (may have to be more robust later)
   int eid=0;
   while(executionMetadata.find(eid) != executionMetadata.end()) { eid++; }
   executionMetadata[eid] = MetaExecution(group_name, execution_name, nameMap);
   executionTreeModel.addGroup(QString::fromStdString(group_name));
   return eid;
}

void ProfilerConductor::addExecution(Execution& e) {
  qDebug() << "addExecution";
  executions << &e;

  executionInfoHash.insert(&e, ExecutionInfo{});

  /// updates titles when a new one becomes known
  connect(&e, SIGNAL(titleKnown()), this, SLOT(updateTitles()));

  /// See if should auto compare the first two execution
  if (GlobalParser::isSet(GlobalParser::auto_compare)) {
    connect(&e, &Execution::doneBuilding, [this]() {

      if (executions.size() < 2) return;

      if (executions[0]->finished && executions[1]->finished) {
        std::cerr << "TWO EXECUTIONS READY\n";
        autoCompareTwoExecution();
      }

    });
  }

  show();
}

void ProfilerConductor::displayExecution(Execution& ex, QString&& title) {
  auto gistMW = executionInfoHash[&ex].gistWindow;
  if (!gistMW) {
    gistMW = createGist(ex, title);
  }
  gistMW->show();
  gistMW->activateWindow();
  connect(gistMW->getCanvas(), &TreeCanvas::announceSelectNode,
          this, [this, &ex](int gid){
            this->tellVisualisationsSelectNode(&ex, gid);
          });
}

// Disabled for now
void ProfilerConductor::updateTitles(void) {
  //for (int i = 0; i < executions.size(); i++) {
  //  ExecutionTreeItem* e = static_cast<ExecutionTreeItem*>(executionTreeModel.index(i,0).internalPointer());
  //  e->updateTitle();
  //}
}

GistMainWindow* ProfilerConductor::createGist(Execution& e, QString title) {
  auto gist = new GistMainWindow{e, this};
  gist->changeTitle(title);
  executionInfoHash[&e].gistWindow = gist;

  connect(gist->getCanvas(), SIGNAL(showNodeInfo(std::string)),
          this, SLOT(showNodeInfoToIDE(std::string)));
  connect(gist->getCanvas(), SIGNAL(showNogood(QString, QString, bool)),
          this, SLOT(showNogoodToIDE(QString, QString, bool)));

  connect(gist->getCanvas(), SIGNAL(searchLogReady(const QString&)),
          this, SLOT(emitSearchLogReady(const QString&)));

  connect(&e, &Execution::doneBuilding,
          gist->getCanvas(), &TreeCanvas::finalize);


  return gist;
}

void ProfilerConductor::gistButtonClicked() {
  auto selectionModel = executionTreeView.selectionModel();
  for (auto ind : selectionModel->selectedIndexes()) {
    Execution* ex = executionTreeModel.getExecution(ind);
    if(ex)
      displayExecution(*ex, QString::fromStdString(ex->getTitle()));
  }
}

QVector<Execution*> ProfilerConductor::getSelectedExecutions() const {
  QVector<Execution*> selected_executions;
  auto selectionModel = executionTreeView.selectionModel();
  for(auto ind : selectionModel->selectedIndexes()) {
    Execution* ex = executionTreeModel.getExecution(ind);
    if(ex)
      selected_executions.append(ex);
  }
  return selected_executions;
}

void ProfilerConductor::compareButtonClicked() {
  QVector<Execution*> selected_executions = getSelectedExecutions();
  if (selected_executions.size() != 2) return;

  auto ex1 = selected_executions[0];
  auto ex2 = selected_executions[1];

  auto gmw1 = executionInfoHash[ex1].gistWindow;
  auto gmw2 = executionInfoHash[ex2].gistWindow;
  if (gmw1 == nullptr || gmw2 == nullptr) return;

  const bool withLabels = compareWithLabelsCB.isChecked();

  /// NOTE(maxim): the new window will delete itself when closed
  int executionId = ex1->getExecutionId();
  MetaExecution& me = executionMetadata[executionId];

  Execution* exec = new Execution;
  exec->setExecutionId(executionId);
  exec->setNameMap(&me.name_map);
  exec->setTitle("Comparison of " + ex1->getTitle() + " with " + ex2->getTitle());

  executionTreeModel.addComparison(&executionTreeView, me, exec);
  auto cmp_tree_dialog = new CmpTreeDialog(this, exec, withLabels, *ex1, *ex2);
  connect(cmp_tree_dialog->getCanvas(), SIGNAL(showNogood(QString, QString, bool)),
          this, SLOT(showNogoodToIDE(QString, QString, bool)));
}

void ProfilerConductor::autoCompareTwoExecution() {
  if (executions.size() < 2) return;

  auto ex1 = executions[0];
  auto ex2 = executions[1];

  /// display the two executions if not already displayed

  // displayExecution(*ex1, "ex1");
  // displayExecution(*ex2, "ex2");

  bool withLabels = true;

  Execution* exec = new Execution{};
  exec->setNameMap(const_cast<NameMap*>(ex1->getNameMap()));
  auto cmp_dialog = new CmpTreeDialog(nullptr, exec, withLabels, *ex1, *ex2);

  cmp_dialog->showResponsibleNogoods();
}

void ProfilerConductor::compareSubtrees() {
  auto selected_executions = getSelectedExecutions();
  if (selected_executions.size() != 2) return;

  const auto* ex1 = selected_executions[0];
  const auto* ex2 = selected_executions[1];

  auto gmw1 = executionInfoHash[ex1].gistWindow;
  auto gmw2 = executionInfoHash[ex2].gistWindow;

  if (gmw1 == nullptr || gmw2 == nullptr) return;

  auto* e = new Execution{};
  addExecution(*e);
  displayExecution(*e, "combined tree");

  subtree_comparison::compareExecutions(*e, *ex1, *ex2);

}

class StatsHelper {
  QString filename;
  StatsHelper(QString filename_) : filename(filename_) {}
};

// using google::protobuf::io::OstreamOutputStream;

// bool writeDelimitedTo(const google::protobuf::MessageLite& message,
//                       google::protobuf::io::ZeroCopyOutputStream* rawOutput) {
//   // We create a new coded stream for each message.  Don't worry, this is fast.
//   google::protobuf::io::CodedOutputStream output(rawOutput);

//   // Write the size.
//   const int size = message.ByteSize();
//   output.WriteVarint32(size);

//   uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
//   if (buffer != nullptr) {
//     // Optimization:  The message fits in one buffer, so use the faster
//     // direct-to-array serialization path.
//     message.SerializeWithCachedSizesToArray(buffer);
//   } else {
//     // Slightly-slower path when the message is multiple buffers.
//     message.SerializeWithCachedSizes(&output);
//     if (output.HadError()) return false;
//   }

//   return true;
// }

void ProfilerConductor::gatherStatisticsClicked() {
  auto selected_executions = getSelectedExecutions();
  for (int i = 0; i < selected_executions.size(); i++) {
    Execution* ex = selected_executions[i];
    GistMainWindow* g = executionInfoHash[ex].gistWindow;

    if (g == nullptr) {
      g = new GistMainWindow(*ex, this);
      executionInfoHash[ex].gistWindow = g;
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
    // QList<QListWidgetItem*> selected = executionList.selectedItems();
    // for (int i = 0; i < selected.size(); i++) {
    //     ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);
    //     Execution& execution = item->execution_;

    //     const char* paths[] = { "webscripts/sunburst.html", "webscripts/icicle.html", "webscripts/variables.html" };
    //     const char* ids[] = { "sunburst", "icicle", "variables" };
    //     std::stringstream ss;
    //     ::collectMLStats(execution.nodeTree().getRoot(), execution.nodeTree().getNA(), &execution, ss);

    //     for (int i = 0 ; i < 3 ; i++) {
    //         if (getWebscriptView(&execution, ids[i]) == NULL) {
    //             QDialog* dialog = new QDialog(this);
    //             WebscriptView* web = new WebscriptView(dialog, paths[i], &execution, ss.str());
    //             registerWebscriptView(&execution, ids[i], web);
            
    //             QHBoxLayout* layout = new QHBoxLayout;
    //             layout->addWidget(web);
    //             dialog->setLayout(layout);

    //             connect(web, &WebscriptView::announceSelectNode,
    //                     this, [this, &execution](int gid){this->tellVisualisationsSelectNode(&execution, gid);});
    //             connect(web, &WebscriptView::announceSelectManyNodes,
    //                     this, [this, &execution](QList<QVariant> gids){
    //                         this->tellVisualisationsSelectManyNodes(&execution, gids);
    //                     });
            
    //             dialog->show();
    //         }

    //         // We get the parent widget here because we want to show()
    //         // the containing QDialog, not the inner WebEngineView.
    //         getWebscriptView(&execution, ids[i])->parentWidget()->show();
    //     }
    // }
}

void ProfilerConductor::tellVisualisationsSelectNode(Execution* execution, int gid) {
    // WebscriptView* p;
    // p = executionInfoHash[execution]->sunburstView;  if (p) p->select(gid);
    // p = executionInfoHash[execution]->icicleView;    if (p) p->select(gid);
    // p = executionInfoHash[execution]->variablesView; if (p) p->select(gid);
    GistMainWindow* g;
    g = executionInfoHash[execution].gistWindow;    if (g) g->selectNode(gid);
}

void ProfilerConductor::tellVisualisationsSelectManyNodes(Execution* execution, QList<QVariant> gids) {
    // WebscriptView* p;
    // p = executionInfoHash[execution]->sunburstView;  if (p) p->selectMany(gids);
    // p = executionInfoHash[execution]->icicleView;    if (p) p->selectMany(gids);
    // p = executionInfoHash[execution]->variablesView; if (p) p->selectMany(gids);
    GistMainWindow* g;
    g = executionInfoHash[execution].gistWindow;    if (g) g->selectManyNodes(gids);
}

void ProfilerConductor::saveExecutionClicked() {

  // QList<QListWidgetItem*> selected = executionList.selectedItems();
  // if (selected.size() != 1) return;
  // auto item = static_cast<ExecutionListItem*>(selected[0]);

  // QString filename =
  //     QFileDialog::getSaveFileName(this, "Save execution", QDir::currentPath());

  // std::ofstream outputFile(filename.toStdString(),
  //                          std::ios::out | std::ios::binary);
  // OstreamOutputStream raw_output(&outputFile);
  // auto& data = item->execution_.getData();

  // message::Node start_node;
  // start_node.set_type(message::Node::START);

  // if (item->execution_.isRestarts()) {
  //   start_node.set_restart_id(0);
  // } else {
  //   start_node.set_restart_id(-1);
  // }

  // start_node.set_info("{\"execution_id\": " + std::to_string(item->execution_.getExecutionId()) + "}");

  // start_node.set_info(item->execution_.getVariableListString());
  // writeDelimitedTo(start_node, &raw_output);

  // for (auto* entry : data.getEntries()) {
  //   message::Node node;
  //   node.set_type(message::Node::NODE);
  //   node.set_sid(entry->s_node_id);
  //   node.set_pid(entry->parent_sid);
  //   node.set_alt(entry->alt);
  //   node.set_kids(entry->numberOfKids);
  //   node.set_status(static_cast<message::Node::NodeStatus>(entry->status));
  //   //            node.set_restart_id
  //   node.set_time(entry->time_stamp);
  //   node.set_thread_id(entry->thread_id);
  //   node.set_label(entry->label);
  //   node.set_domain_size(entry->domain);
  //   //            node.set_solution(entry->);
  //   auto ngit = data.getNogoods().find(entry->s_node_id);
  //   if (ngit != data.getNogoods().end()) node.set_nogood(ngit->second.original);
  //   auto infoit = data.sid2info.find(entry->s_node_id);
  //   if (infoit != data.sid2info.end()) node.set_info(*infoit->second);
  //   writeDelimitedTo(node, &raw_output);
  // }

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
  // loadSaved(e, filename);
}

void ProfilerConductor::deleteExecutionClicked() {
  auto selected_executions = getSelectedExecutions();
  for (int i = 0; i < selected_executions.size(); i++) {
    auto ex = selected_executions[i];
    executions.removeOne(ex);
    if (executionInfoHash[ex].gistWindow != NULL) {
        executionInfoHash[ex].gistWindow->stopReceiver();
        delete executionInfoHash[ex].gistWindow;
    }
    delete ex;
  }
}

WebscriptView*
ProfilerConductor::getWebscriptView(Execution* execution, std::string id) {
    // if      (id == "sunburst")  return executionInfoHash[execution]->sunburstView;
    // else if (id == "icicle")    return executionInfoHash[execution]->icicleView;
    // else if (id == "variables") return executionInfoHash[execution]->variablesView;
    return NULL;
}

void ProfilerConductor::registerWebscriptView(Execution* execution, std::string id, WebscriptView* webView) {
    // if      (id == "sunburst")  executionInfoHash[execution]->sunburstView = webView;
    // else if (id == "icicle")    executionInfoHash[execution]->icicleView = webView;
    // else if (id == "variables") executionInfoHash[execution]->variablesView = webView;
}

void ProfilerConductor::createDebugExecution() {
  auto e = new Execution{};
  addExecution(*e);

  //// NOTE(maxim): quick workaround for now:
  auto eid = getNextExecId("External Execution", e->getTitle(), NameMap());
  MetaExecution& me = executionMetadata[eid];
  executionTreeModel.addExecution(&executionTreeView, executionMetadata[eid], e);
}

void ProfilerConductor::cloneExecution() {
  auto selectionModel = executionTreeView.selectionModel();
  auto selected = selectionModel->selectedIndexes();
  auto sel_ex = executionTreeModel.getExecution(selected.first());
  //TODO
}

void ProfilerConductor::createExecution(std::unique_ptr<NodeTree> nt,
                                        std::unique_ptr<Data> data) {

  auto e = new Execution(std::move(nt), std::move(data));
  addExecution(*e);

  e->setTitle("Extracted Execution");

  auto gist = createGist(*e, e->getTitle().c_str());
}

void ProfilerConductor::executionIdReady(Execution* e) {
  // latest_execution = e; // TODO(maxim): check if still needed
  int eid = e->getExecutionId();
  auto eit = executionMetadata.find(eid);
  if(eit == executionMetadata.end())
    eid = getNextExecId("External Execution", e->getTitle(), NameMap());
  MetaExecution& me = executionMetadata[eid];
  e->setNameMap(&me.name_map);

  executionTreeModel.addExecution(&executionTreeView, executionMetadata[eid], e);
  executionTreeView.update();

  e->has_exec_id = true;
  e->has_exec_id_cond.wakeOne();
}

void ProfilerConductor::executionStarted(Execution* e) {
  addExecution(*e);
}

void ProfilerConductor::showNodeInfoToIDE(std::string extra_info) {
    emit(showNodeInfo(extra_info));
}

void ProfilerConductor::showNogoodToIDE(QString heatmap, QString text, bool record) {
    emit showNogood(heatmap, text, record);
}

void ProfilerConductor::emitSearchLogReady(const QString& path) {
    emit searchLogReady(path);
}

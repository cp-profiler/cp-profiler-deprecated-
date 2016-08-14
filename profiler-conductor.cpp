#include "profiler-conductor.hh"
#include "profiler-tcp-server.hh"
#include "gistmainwindow.h"
#include "cmp_tree_dialog.hh"
#include "data.hh"

#include "globalhelper.hh"
#include "libs/perf_helper.hh"
#include "treecanvas.hh"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

#include "message.pb.hh"

#include "ml-stats.hh"
#include "webscript.hh"

#include <fstream>

#include <QPushButton>
#include <QVBoxLayout>

#include <thread>
#include <chrono>

#include <QDebug>

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
    case message::Node::NODE:
        e->handleNewNode(msg);
        break;
    case message::Node::START:
        if (msg.has_info()) {
            e->setVariableListString(msg.info());
        }
        break;
    }
  }
  e->doneReceiving();
  return e;
}

ProfilerConductor::ProfilerConductor() : QMainWindow() {
  // this->setMinimumHeight(320);
  // this->setMinimumWidth(320);

  /// NOTE(maxim): required by the comparison script
  // std::cout << "READY TO LISTEN" << std::endl;

  QWidget* centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  executionList = new QListWidget;
  executionList->setSelectionMode(QAbstractItemView::MultiSelection);

  QPushButton* gistButton = new QPushButton("show tree");
  connect(gistButton, SIGNAL(clicked(bool)), this,
          SLOT(gistButtonClicked(bool)));

  QPushButton* compareButton = new QPushButton("compare trees");
  connect(compareButton, SIGNAL(clicked(bool)), this,
          SLOT(compareButtonClicked(bool)));

  compareWithLabelsCB = new QCheckBox("with labels");

  QPushButton* gatherStatisticsButton = new QPushButton("gather statistics");
  connect(gatherStatisticsButton, SIGNAL(clicked(bool)), this,
          SLOT(gatherStatisticsClicked(bool)));

  QPushButton* webscriptButton = new QPushButton("webscript view");
  connect(webscriptButton, SIGNAL(clicked(bool)), this,
          SLOT(webscriptClicked(bool)));

  QPushButton* saveExecutionButton = new QPushButton("save execution");
  connect(saveExecutionButton, SIGNAL(clicked(bool)), this,
          SLOT(saveExecutionClicked(bool)));

  QPushButton* loadExecutionButton = new QPushButton("load execution");
  connect(loadExecutionButton, SIGNAL(clicked(bool)), this,
          SLOT(loadExecutionClicked(bool)));

  QPushButton* deleteExecutionButton = new QPushButton("delete execution");
  connect(deleteExecutionButton, SIGNAL(clicked(bool)), this,
          SLOT(deleteExecutionClicked(bool)));

  QPushButton* debugExecutionButton = new QPushButton("debug execution");
  connect(debugExecutionButton, SIGNAL(clicked()), this,
          SLOT(createDebugExecution()));

  QGridLayout* layout = new QGridLayout();

  layout->addWidget(executionList, 0, 0, 1, 2);
  layout->addWidget(gistButton, 1, 0, 1, 2);

  layout->addWidget(compareButton, 2, 0, 1, 1);
  layout->addWidget(compareWithLabelsCB, 2, 1, 1, 1);

  layout->addWidget(gatherStatisticsButton, 3, 0, 1, 2);
  layout->addWidget(webscriptButton, 4, 0, 1, 2);
  layout->addWidget(saveExecutionButton, 5, 0, 1, 2);
  layout->addWidget(loadExecutionButton, 6, 0, 1, 2);
  layout->addWidget(deleteExecutionButton, 7, 0, 1, 2);

#ifdef MAXIM_DEBUG
  layout->addWidget(debugExecutionButton, 7, 0, 1, 2);
#endif

  centralWidget->setLayout(layout);

  // Listen for new executions.
  ProfilerTcpServer* listener = new ProfilerTcpServer(this);
  listener->listen(QHostAddress::Any, 6565);
}

class ExecutionListItem : public QListWidgetItem {
 public:
  ExecutionListItem(Execution* execution, QListWidget* parent, int type = Type)
      : QListWidgetItem(parent, type),
        execution_(execution)
    {}

  Execution* execution_;
};

void ProfilerConductor::newExecution(Execution* execution) {
  ExecutionListItem* newItem = new ExecutionListItem(execution, executionList);
  newItem->setText("some execution");
  newItem->setSelected(true);
  executionList->addItem(newItem);
  executions << execution;

  executionInfoHash.insert(execution, new ExecutionInfo);

  connect(execution, SIGNAL(titleKnown()), this, SLOT(updateList()));
  connect(execution, SIGNAL(doneReceiving()), this,
          SLOT(onSomeFinishedReceiving()));
  connect(execution, SIGNAL(doneBuilding()), this,
          SLOT(onSomeFinishedBuilding()));
  show();
}

void ProfilerConductor::updateList(void) {
  for (int i = 0; i < executions.size(); i++) {
    executionList->item(i)
        ->setText(QString::fromStdString(executions[i]->getTitle()));
  }
}

void ProfilerConductor::gistButtonClicked(bool) {
  QList<QListWidgetItem*> selected = executionList->selectedItems();
  for (int i = 0; i < selected.size(); i++) {
    ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);
    Execution* execution = item->execution_;
    GistMainWindow* gistMW = executionInfoHash[execution]->gistWindow;
    if (gistMW == nullptr) {
      gistMW = new GistMainWindow(execution, this);
      gistMW->changeTitle(item->text());
      executionInfoHash[execution]->gistWindow = gistMW;
    }
    gistMW->show();
    gistMW->activateWindow();
    connect(item->execution_, SIGNAL(doneReceiving()), gistMW,
            SIGNAL(doneReceiving()));
    connect(gistMW->getGist()->getCanvas(), &TreeCanvas::announceSelectNode,
            this, [this, execution](int gid){this->tellVisualisationsSelectNode(execution, gid);});
  }
}

void ProfilerConductor::compareButtonClicked(bool) { compareExecutions(false); }

void ProfilerConductor::compareExecutions(bool auto_save) {
  /// NOTE(maxim): compare with labels when using a script (auto_save == true)
  const bool withLabels = compareWithLabelsCB->isChecked() || auto_save;

  QList<QListWidgetItem*> selected = executionList->selectedItems();
  if (selected.size() != 2) return;
  ExecutionListItem* item1 = static_cast<ExecutionListItem*>(selected[0]);
  ExecutionListItem* item2 = static_cast<ExecutionListItem*>(selected[1]);
  Execution* e = new Execution;

  GistMainWindow* gmw1 = executionInfoHash[item1->execution_]->gistWindow;
  GistMainWindow* gmw2 = executionInfoHash[item2->execution_]->gistWindow;
  if (gmw1 == nullptr || gmw2 == nullptr) return;

  CmpTreeDialog* ctd = new CmpTreeDialog(
      this, e, withLabels, *gmw1->getGist()->getExecution(),
      *gmw2->getGist()->getExecution());
  (void)ctd;

  // if (auto_save) {
  //     ctd->saveComparisonStatsTo("/home/maxim/temp_stats.txt");
  // }
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

void ProfilerConductor::gatherStatisticsClicked(bool) {
  QList<QListWidgetItem*> selected = executionList->selectedItems();
  for (int i = 0; i < selected.size(); i++) {
    ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);

    GistMainWindow* g = executionInfoHash[item->execution_]->gistWindow;

    if (g == nullptr) {
      g = new GistMainWindow(item->execution_, this);
      executionInfoHash[item->execution_]->gistWindow = g;
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

void ProfilerConductor::webscriptClicked(bool) {
    QList<QListWidgetItem*> selected = executionList->selectedItems();
    for (int i = 0; i < selected.size(); i++) {
        ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);
        Execution* execution = item->execution_;

        const char* paths[] = { "webscripts/sunburst.html", "webscripts/icicle.html", "webscripts/variables.html" };
        const char* ids[] = { "sunburst", "icicle", "variables" };
        std::stringstream ss;
        ::collectMLStats(execution->getRootNode(), execution->getNA(), execution, ss);

        for (int i = 0 ; i < 3 ; i++) {
            if (getWebscriptView(execution, ids[i]) == NULL) {
                QDialog* dialog = new QDialog(this);
                WebscriptView* web = new WebscriptView(dialog, paths[i], execution, ss.str());
                registerWebscriptView(execution, ids[i], web);
            
                QHBoxLayout* layout = new QHBoxLayout;
                layout->addWidget(web);
                dialog->setLayout(layout);

                connect(web, &WebscriptView::announceSelectNode,
                        this, [this, execution](int gid){this->tellVisualisationsSelectNode(execution, gid);});
                connect(web, &WebscriptView::announceSelectManyNodes,
                        this, [this, execution](QList<QVariant> gids){
                            this->tellVisualisationsSelectManyNodes(execution, gids);
                        });
            
                dialog->show();
            }

            // We get the parent widget here because we want to show()
            // the containing QDialog, not the inner WebEngineView.
            getWebscriptView(execution, ids[i])->parentWidget()->show();
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

void ProfilerConductor::onSomeFinishedBuilding() {
  /// ***** Save Search Log *****
  if (GlobalParser::isSet(GlobalParser::save_log)) {
    if (executions.size() == 1) {
      auto item = static_cast<ExecutionListItem*>(executionList->item(0));

      auto file_name = GlobalParser::value(GlobalParser::save_log);

      GistMainWindow* g = executionInfoHash[item->execution_]->gistWindow;
      g->getGist()->getCanvas()->printSearchLogTo(file_name);
    }
  }

  if (GlobalParser::isSet(GlobalParser::auto_compare)) {
    if (executions.size() == 2) {
      compareExecutions(true);
    }
  }

  if (GlobalParser::isSet(GlobalParser::auto_stats)) {
    if (executions.size() == 1) {
      auto item = static_cast<ExecutionListItem*>(executionList->item(0));
      auto file_name = GlobalParser::value(GlobalParser::auto_stats);
      GistMainWindow* g = executionInfoHash[item->execution_]->gistWindow;
      g->setStatsFilename(file_name);
      g->gatherStatistics();
      // bye bye
      qDebug() << "auto stats, terminate";
      qApp->exit();
    }
  }
}

void ProfilerConductor::onSomeFinishedReceiving() {
  // NOTE(maixm): if running in a script mode, build the trees
  // immediately after the data is fully received
  if (GlobalParser::isSet(GlobalParser::save_log)) {
    if (executions.size() == 1) {
      gistButtonClicked(true);
    }
  }

  if (GlobalParser::isSet(GlobalParser::auto_compare)) {
    if (executions.size() == 2) {
      gistButtonClicked(true);
    }
  }

  if (GlobalParser::isSet(GlobalParser::auto_stats)) {
    if (executions.size() == 1) {
      gistButtonClicked(true);
    }
  }
}

void ProfilerConductor::saveExecutionClicked(bool) {
  QList<QListWidgetItem*> selected = executionList->selectedItems();
  if (selected.size() != 1) return;
  ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[0]);

  QString filename =
      QFileDialog::getSaveFileName(this, "Save execution", QDir::currentPath());
  std::ofstream outputFile(filename.toStdString(),
                           std::ios::out | std::ios::binary);
  OstreamOutputStream raw_output(&outputFile);
  Data* data = item->execution_->getData();

  std::vector<unsigned int> keys;
  for (auto it = data->gid2entry.begin(); it != data->gid2entry.end(); it++) {
    keys.push_back(it->first);
  }
  sort(keys.begin(), keys.end());

  message::Node start_node;
  start_node.set_type(message::Node::START);
  start_node.set_info(item->execution_->getVariableListString());
  writeDelimitedTo(start_node, &raw_output);

  for (auto it = keys.begin(); it != keys.end(); it++) {
    DbEntry* entry = data->gid2entry[*it];
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
    auto ngit = data->getNogoods().find(entry->s_node_id);
    if (ngit != data->getNogoods().end()) node.set_nogood(ngit->second);
    auto infoit = data->sid2info.find(entry->s_node_id);
    if (infoit != data->sid2info.end()) node.set_info(*infoit->second);
    writeDelimitedTo(node, &raw_output);
  }
}

void ProfilerConductor::loadExecutionClicked(bool) {
  QString filename =
      QFileDialog::getOpenFileName(this, "Load execution", QDir::currentPath());
  if (!filename.isNull())
      loadExecution(filename.toStdString());
}



void ProfilerConductor::loadExecution(std::string filename) {
  Execution* e = new Execution();
  newExecution(e);
  /// TODO(maxim): should somehow know if it was restarts, TRUE for now
  e->start("loaded from " + filename, true);
  loadSaved(e, filename);
}

void ProfilerConductor::deleteExecutionClicked(bool checked) {
  (void)checked;
  QList<QListWidgetItem*> selected = executionList->selectedItems();
  for (int i = 0; i < selected.size(); i++) {
    ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);
    executions.removeOne(item->execution_);
    delete item->execution_;
    if (executionInfoHash[item->execution_]->gistWindow != NULL) {
        executionInfoHash[item->execution_]->gistWindow->stopReceiver();
        delete executionInfoHash[item->execution_]->gistWindow;
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
  newExecution(e);

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

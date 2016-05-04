#include "profiler-conductor.hh"
#include "receiverthread.hh"
#include "profiler-tcp-server.hh"
#include "gistmainwindow.h"
#include "cmp_tree_dialog.hh"

#include "globalhelper.hh"
#include "treecanvas.hh"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

#include "message.pb.hh"

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

Execution* loadSaved(std::string path) {
  Execution* e = new Execution();
  std::ifstream inputFile(path, std::ios::in | std::ios::binary);
  IstreamInputStream raw_input(&inputFile);
  while (true) {
    message::Node msg;
    bool ok = readDelimitedFrom(&raw_input, &msg);
    if (!ok) break;
    e->handleNewNode(msg);
  }
  e->doneReceiving();
  return e;
}

ProfilerConductor::ProfilerConductor() : QMainWindow() {
  // this->setMinimumHeight(320);
  // this->setMinimumWidth(320);

  /// NOTE(maxim): required by the comparison script
  std::cout << "READY TO LISTEN" << std::endl;

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

  QPushButton* saveExecutionButton = new QPushButton("save execution");
  connect(saveExecutionButton, SIGNAL(clicked(bool)), this,
          SLOT(saveExecutionClicked(bool)));

  QPushButton* loadExecutionButton = new QPushButton("load execution");
  connect(loadExecutionButton, SIGNAL(clicked(bool)), this,
          SLOT(loadExecutionClicked(bool)));

  QPushButton* deleteExecutionButton = new QPushButton("delete execution");
  connect(deleteExecutionButton, SIGNAL(clicked(bool)), this,
          SLOT(deleteExecutionClicked(bool)));

  QGridLayout* layout = new QGridLayout();

  layout->addWidget(executionList, 0, 0, 1, 2);
  layout->addWidget(gistButton, 1, 0, 1, 2);

  layout->addWidget(compareButton, 2, 0, 1, 1);
  layout->addWidget(compareWithLabelsCB, 2, 1, 1, 1);

  layout->addWidget(gatherStatisticsButton, 3, 0, 1, 2);
  layout->addWidget(saveExecutionButton, 4, 0, 1, 2);
  layout->addWidget(loadExecutionButton, 5, 0, 1, 2);
  layout->addWidget(deleteExecutionButton, 6, 0, 1, 2);

  centralWidget->setLayout(layout);

  // Listen for new executions.
  ProfilerTcpServer* listener = new ProfilerTcpServer(this);
  listener->listen(QHostAddress::Any, 6565);
}

class ExecutionListItem : public QListWidgetItem {
 public:
  ExecutionListItem(Execution* execution, QListWidget* parent, int type = Type)
      : QListWidgetItem(parent, type),
        execution_(execution),
        gistWindow_(nullptr) {}

  Execution* execution_;
  GistMainWindow* gistWindow_;
};

void ProfilerConductor::newExecution(Execution* execution) {
  ExecutionListItem* newItem = new ExecutionListItem(execution, executionList);
  newItem->setText("some execution");
  newItem->setSelected(true);
  executionList->addItem(newItem);
  executions << execution;

  connect(execution, SIGNAL(titleKnown()), this, SLOT(updateList()));
  connect(execution, SIGNAL(doneReceiving()), this,
          SLOT(onSomeFinishedReceiving()));
  connect(execution, SIGNAL(doneBuilding()), this,
          SLOT(onSomeFinishedBuilding()));
}

void ProfilerConductor::updateList(void) {
  for (int i = 0; i < executions.size(); i++) {
    executionList->item(i)
        ->setText(QString::fromStdString(executions[i]->getDescription()));
  }
}

void ProfilerConductor::gistButtonClicked(bool) {
  QList<QListWidgetItem*> selected = executionList->selectedItems();
  for (int i = 0; i < selected.size(); i++) {
    ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);
    GistMainWindow* gistMW = item->gistWindow_;
    if (gistMW == nullptr) {
      gistMW = new GistMainWindow(item->execution_, this);
      item->gistWindow_ = gistMW;
      gistMW->changeTitle(item->text());
    }
    gistMW->show();
    gistMW->activateWindow();
    connect(item->execution_, SIGNAL(doneReceiving()), gistMW,
            SIGNAL(doneReceiving()));
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

  if (item1->gistWindow_ == nullptr && item2->gistWindow_ == nullptr) return;

  CmpTreeDialog* ctd = new CmpTreeDialog(
      this, e, withLabels, item1->gistWindow_->getGist()->getCanvas(),
      item2->gistWindow_->getGist()->getCanvas());

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
    GistMainWindow* g = item->gistWindow_;
    if (g == nullptr) {
      g = new GistMainWindow(item->execution_, this);
      item->gistWindow_ = g;
    }
    //        g->hide();

    // This is a big mess.  The user must wait for the tree to be
    // built before they select their file.

    QString filename = QFileDialog::getSaveFileName(this, "Save statistics",
                                                    QDir::currentPath());
    // Very inelegant
    g->setStatsFilename(filename);

    g->gatherStatistics();

    // connect(g, SIGNAL(buildingFinished(void)),
    //         g, SLOT(gatherStatistics(void)));
    // g->show();
    // g->activateWindow();
  }
}

void ProfilerConductor::onSomeFinishedBuilding() {
  /// ***** Save Search Log *****
  if (GlobalParser::isSet(GlobalParser::save_log)) {
    if (executions.size() == 1) {
      auto item = static_cast<ExecutionListItem*>(executionList->item(0));

      auto file_name = GlobalParser::value(GlobalParser::save_log);

      GistMainWindow* g = item->gistWindow_;
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
      GistMainWindow* g = item->gistWindow_;
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
    auto ngit = data->sid2nogood.find(entry->s_node_id);
    if (ngit != data->sid2nogood.end()) node.set_nogood(ngit->second);
    auto infoit = data->sid2info.find(entry->s_node_id);
    if (infoit != data->sid2info.end()) node.set_info(*infoit->second);
    writeDelimitedTo(node, &raw_output);
  }
}

void ProfilerConductor::loadExecutionClicked(bool) {
  QString filename =
      QFileDialog::getOpenFileName(this, "Load execution", QDir::currentPath());
  loadExecution(filename.toStdString());
}

void ProfilerConductor::loadExecution(std::string filename) {
  Execution* e = loadSaved(filename);
  newExecution(e);
  /// TODO(maxim): should somehow know if it was restarts, TRUE for now
  e->start("loaded from " + filename, true);
}

void ProfilerConductor::deleteExecutionClicked(bool checked) {
  (void)checked;
  QList<QListWidgetItem*> selected = executionList->selectedItems();
  for (int i = 0; i < selected.size(); i++) {
    ExecutionListItem* item = static_cast<ExecutionListItem*>(selected[i]);
    executions.removeOne(item->execution_);
    delete item->execution_;
    item->gistWindow_->stopReceiver();
    delete item->gistWindow_;
    delete item;
  }
}

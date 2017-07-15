#ifndef PROFILER_CONDUCTOR_HH
#define PROFILER_CONDUCTOR_HH

#include <QListWidget>
#include <QCheckBox>
#include <QMainWindow>

#include <memory>
#include <unordered_map>
#include "execution.hh"
#include "namemap.hh"

#include <utility>

class WebscriptView;
class GistMainWindow;
class NodeTree;
class Data;

class ProfilerTcpServer;

class ExecutionInfo {
public:
    WebscriptView* sunburstView  = nullptr;
    WebscriptView* icicleView    = nullptr;
    WebscriptView* variablesView = nullptr;
    GistMainWindow* gistWindow   = nullptr;
};

class ProfilerConductor : public QMainWindow {
  Q_OBJECT
 private:
  QList<Execution*> executions;
  QHash<const Execution*, ExecutionInfo*> executionInfoHash;

  quint16 listen_port;

  QListWidget executionList;
  QCheckBox   compareWithLabelsCB;

  QVector<NameMap> nameMaps;

  /// NOTE(maxim): This is a bit hacky
  Execution* latest_execution = nullptr;

  std::unique_ptr<ProfilerTcpServer> listener;

  void addExecution(Execution& execution);
  void displayExecution(Execution& execution, QString&& title);
  GistMainWindow* createGist(Execution&, QString title);
  WebscriptView* getWebscriptView(Execution* execution, std::string id);
  void registerWebscriptView(Execution* execution, std::string id, WebscriptView* webView);
  void tellVisualisationsSelectNode(Execution* execution, int gid);
  void tellVisualisationsSelectManyNodes(Execution* execution, QList<QVariant> gids);

 private slots:
  void gistButtonClicked();
  void compareButtonClicked();
  void compareSubtrees();
  void gatherStatisticsClicked();
  void webscriptClicked();
  void saveExecutionClicked();
  void loadExecutionClicked();
  void deleteExecutionClicked();
  void createDebugExecution();
  void updateTitles();
  void cloneExecution();
  void executionIdReady(Execution* e);

 public:
  ProfilerConductor();
  ~ProfilerConductor();

  int getNextExecId(const NameMap& nameMap);
  int getListenPort();
  void loadExecution(std::string filename);
  void createExecution(std::unique_ptr<NodeTree> nt, std::unique_ptr<Data>);

  void autoCompareTwoExecution();

Q_SIGNALS:
  void showNodeInfo(std::string extra_info);
  void showNogood(QString heatmap, QString text, bool record);

 public Q_SLOTS:
  void showNodeInfoToIDE(std::string extra_info);
  void showNogoodToIDE(QString heatmap, QString text, bool record);

};

#endif

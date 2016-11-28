#ifndef PROFILER_CONDUCTOR_HH
#define PROFILER_CONDUCTOR_HH

#include <QListWidget>
#include <QCheckBox>
#include <QMainWindow>

#include <memory>

class Execution;
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

  QListWidget executionList;
  QCheckBox   compareWithLabelsCB;

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

 public:
  ProfilerConductor();
  ~ProfilerConductor();

  void loadExecution(std::string filename);
  void createExecution(std::unique_ptr<NodeTree> nt, std::unique_ptr<Data>);
};

#endif

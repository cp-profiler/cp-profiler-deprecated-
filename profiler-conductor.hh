#ifndef PROFILER_CONDUCTOR_HH
#define PROFILER_CONDUCTOR_HH

#include <QListWidget>
#include <QMainWindow>

class Execution;
class QCheckBox;
class WebscriptView;
class GistMainWindow;

class ExecutionInfo {
public:
    WebscriptView* sunburstView;
    WebscriptView* icicleView;
    WebscriptView* variablesView;
    GistMainWindow* gistWindow;

    ExecutionInfo()
        : sunburstView(NULL),
          icicleView(NULL),
          variablesView(NULL),
          gistWindow(NULL)
    {}
};

class ProfilerConductor : public QMainWindow {
  Q_OBJECT
 private:
  QList<Execution*> executions;
  QHash<Execution*, ExecutionInfo*> executionInfoHash;

  QListWidget* executionList;
  QCheckBox* compareWithLabelsCB;
 private slots:
  void gistButtonClicked(bool checked);
  void compareButtonClicked(bool checked);
  void gatherStatisticsClicked(bool checked);
  void webscriptClicked(bool checked);
  void saveExecutionClicked(bool checked);
  void loadExecutionClicked(bool checked);
  void deleteExecutionClicked(bool checked);
  void createDebugExecution();

 public:
  ProfilerConductor();
  void newExecution(Execution* execution);
  void loadExecution(std::string filename);
  void compareExecutions(bool auto_save);

  void registerWebscriptView(Execution* execution, std::string id, WebscriptView* webView);
  WebscriptView* getWebscriptView(Execution* execution, std::string id);
  void tellVisualisationsSelectNode(Execution* execution, int gid);
  void tellVisualisationsSelectManyNodes(Execution* execution, QList<QVariant> gids);
  
 public slots:
  void updateList();
  void onSomeFinishedReceiving();
  void onSomeFinishedBuilding();
};

#endif

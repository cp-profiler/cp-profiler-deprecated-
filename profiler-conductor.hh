#ifndef PROFILER_CONDUCTOR_HH
#define PROFILER_CONDUCTOR_HH

#include <QListWidget>
#include <QMainWindow>

#include "execution.hh"

class ProfilerConductor : public QMainWindow {
    Q_OBJECT
private:
    QListWidget* executionList;
    QList<Execution*> executions;
private slots:
    void gistButtonClicked(bool checked);
    void compareButtonClicked(bool checked);
    void gatherStatisticsClicked(bool checked);
    void saveExecutionClicked(bool checked);
    void loadExecutionClicked(bool checked);
public:
    ProfilerConductor();
    void newExecution(Execution* execution);
    void loadExecution(std::string filename);
public slots:
    void updateList();
};

#endif

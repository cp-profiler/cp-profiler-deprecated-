#ifndef PROFILER_CONDUCTOR_HH
#define PROFILER_CONDUCTOR_HH

#include <QListWidget>
#include <QMainWindow>

#include "execution.hh"

class QCheckBox;

class ProfilerConductor : public QMainWindow {
    Q_OBJECT
private:
    QListWidget* executionList;
    QCheckBox* compareWithLabelsCB;
    QList<Execution*> executions;
private slots:
    void gistButtonClicked(bool checked);
    void compareButtonClicked(bool checked);
    void gatherStatisticsClicked(bool checked);
    void saveExecutionClicked(bool checked);
    void loadExecutionClicked(bool checked);
    void deleteExecutionClicked(bool checked);
public:
    ProfilerConductor();
    void newExecution(Execution* execution);
    void loadExecution(std::string filename);
    void compareExecutions(bool auto_save);
public slots:
    void updateList();
    void onSomeFinishedReceiving();
    void onSomeTreeReady();
};

#endif

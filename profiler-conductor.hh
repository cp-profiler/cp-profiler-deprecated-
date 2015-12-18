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
public:
    ProfilerConductor();
    void newExecution(Execution* execution);
public slots:
    void updateList();
};

#endif

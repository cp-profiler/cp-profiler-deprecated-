#ifndef PROFILER_CONDUCTOR_HH
#define PROFILER_CONDUCTOR_HH

#include <QListWidget>
#include <QMainWindow>

#include "execution.hh"

// class ExecutionListModel : public QAbstractListModel {
//     Q_OBJECT

// public:
//     ExecutionListModel(QObject* parent = 0)
//         : QAbstractListModel(parent) {}

//     void addExecution(Execution* execution) {
//         executions << execution;
//         // QModelIndex idx = index(executions.size()-1);
//         // std::cerr << "emitting dataChanged: " << idx.row() << "\n";
//         beginInsertRows(
//         insertRows(executions.size()-1, 1);
//         // emit dataChanged(idx, idx);
//     }
//     int rowCount(const QModelIndex& parent) const {
//         return executions.size();
//     }
//     QVariant data(const QModelIndex& index, int role) const {
//         return QVariant("some data");
//     }

// private:
//     QList<Execution*> executions;
// };

// class ExecutionListItem : public QListWidgetItem {
//     Q_OBJECT
// private:
//     Execution* execution;
// public:
//     ExecutionListItem(Execution* execution, QListWidget* parent = 0, int type = Type)
//         : QListWidgetItem(parent, type), execution(execution) {}
// public slots:
//     void updateItem(void) {
//         setText("something new");
//     }
// };
    

class ProfilerConductor : public QMainWindow {
    Q_OBJECT
private:
    QListWidget* executionList;
    QList<Execution*> executions;
    // ExecutionListModel* executionListModel;
// private Q_SLOTS:
//     void newSolverConnection();
public:
    ProfilerConductor();
    void newExecution(Execution* execution);
public slots:
    void updateList();
};

#endif

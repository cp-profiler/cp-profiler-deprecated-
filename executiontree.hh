#ifndef EXECUTIONTREE_H
#define EXECUTIONTREE_H

#include <QString>
#include <QStandardItemModel>
#include <QTreeView>
#include "execution.hh"
#include "namemap.hh"

struct MetaExecution {
  std::string group_name;
  std::string execution_name;
  NameMap name_map;
  MetaExecution(const std::string& gn,
                const std::string& en,
                const NameMap& nm) :
    group_name(gn), execution_name(en), name_map(nm) {}
  MetaExecution() {}
};

class GroupTreeItem : public QStandardItem {
private:
  QString group_name;
public:
  GroupTreeItem(const QString& groupName);
  const QString& getGroupName(void) const;
  void setGroupName(const QString& groupName);
};

class ExecutionTreeItem : public QStandardItem {
private:
  Execution* execution;
public:
  ExecutionTreeItem(Execution* e);
  Execution* getExecution(void) const;
};

class ComparisonTreeItem : public QStandardItem {
public:
  ComparisonTreeItem();
};

class ExecutionTreeModel : public QStandardItemModel {
  Q_OBJECT

public:
  explicit ExecutionTreeModel(QObject* parent = nullptr);
  ~ExecutionTreeModel();

  ComparisonTreeItem* findComparison(const MetaExecution& me) const;
  ComparisonTreeItem* addComparison(QTreeView* tv, const MetaExecution& me, Execution* ei);

  GroupTreeItem* findGroup(const QString& groupName) const;
  GroupTreeItem* addGroup(const QString& groupName);

  Execution* getExecution(const QModelIndex& ind) const;
  void addExecution(QTreeView* tv, const MetaExecution& me, Execution* ei, QStandardItem* parent = nullptr);
};

#endif // EXECUTIONTREE_H

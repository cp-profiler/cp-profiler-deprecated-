
#include "executiontree.hh"

/// ########################################## ///
///   GroupTreeItem ///
/// ########################################## ///

GroupTreeItem::GroupTreeItem(const QString& groupName) :
    group_name(groupName) {
  setText(groupName);
}

const QString& GroupTreeItem::getGroupName(void) const {
  return group_name;
}

void GroupTreeItem::setGroupName(const QString& groupName) {
  group_name = groupName;
}

/// ########################################## ///
///   ExecutionTreeItem ///
/// ########################################## ///
ExecutionTreeItem::ExecutionTreeItem(Execution* e) : execution(e) {
  setText(QString::number(e->getExecutionId()) + ": " + QString::fromStdString(e->getTitle()));
}

Execution* ExecutionTreeItem::getExecution(void) const {
  return execution;
}

/// ########################################## ///
///   ComparisonTreeItem ///
/// ########################################## ///
ComparisonTreeItem::ComparisonTreeItem() {
  setText("Comparisons");
}

/// ########################################## ///
///   ExecutionTreeModel ///
/// ########################################## ///

ExecutionTreeModel::ExecutionTreeModel(QObject* parent) :
    QStandardItemModel(parent) {}

ExecutionTreeModel::~ExecutionTreeModel() {}

void ExecutionTreeModel::addExecution(QTreeView* tv, const MetaExecution& me,
                                      Execution* e,  QStandardItem* parent) {
  QStandardItem* parentItem;
  if(parent) {
    parentItem = parent;
  } else {
    const QString group_name = QString::fromStdString(me.group_name);
    parentItem = findGroup(group_name);
    if(!parentItem) parentItem = addGroup(group_name);
  }

  ExecutionTreeItem* item = new ExecutionTreeItem(e);
  parentItem->appendRow(item);

  tv->expand(indexFromItem(item));
  tv->expandAll();
}

Execution* ExecutionTreeModel::getExecution(const QModelIndex& ind) const {
  ExecutionTreeItem* eti = dynamic_cast<ExecutionTreeItem*>(itemFromIndex(ind));
  if(eti) return eti->getExecution();
  return nullptr;
}

ComparisonTreeItem* ExecutionTreeModel::findComparison(const MetaExecution& me) const {
  QStandardItem* groupItem = findGroup(QString::fromStdString(me.group_name));
  if(groupItem) {
    int rows = groupItem->rowCount();
    for(int i=0; i<rows; i++) {
      ComparisonTreeItem* cti = dynamic_cast<ComparisonTreeItem*>(groupItem->child(i, 0));
      if(cti) return cti;
    }
  }
  return nullptr;
}

ComparisonTreeItem* ExecutionTreeModel::addComparison(QTreeView* tv,
                                                      const MetaExecution& me,
                                                      Execution* e) {
  ComparisonTreeItem* item = findComparison(me);
  if(!item) {
    GroupTreeItem* groupItem = findGroup(QString::fromStdString(me.group_name));
    if(!groupItem) {
      QString orphan_group = "Orphan Comparison";
      groupItem = findGroup(orphan_group);
      if(!groupItem)
        groupItem = addGroup(orphan_group);
    }
    item = new ComparisonTreeItem;
    groupItem->appendRow(item);
  }

  addExecution(tv, me, e, item);

  return item;
}

GroupTreeItem* ExecutionTreeModel::findGroup(const QString& groupName) const {
  int rows = rowCount();
  for(int i=0; i<rows; i++) {
    GroupTreeItem* eti = dynamic_cast<GroupTreeItem*>(invisibleRootItem()->child(i, 0));
    if(eti && eti->getGroupName() == groupName) return eti;
  }
  return nullptr;
}

GroupTreeItem* ExecutionTreeModel::addGroup(const QString& groupName) {
  GroupTreeItem* item = findGroup(groupName);
  if(!item) {
    item = new GroupTreeItem(groupName);
    appendRow(item);
  }
  return item;
}

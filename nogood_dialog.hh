#ifndef NOGOOD_DIALOG_HH
#define NOGOOD_DIALOG_HH

#include <QDialog>
#include <QDebug>
#include <QTableWidget>
#include <unordered_map>
#include <vector>
#include <QSortFilterProxyModel>

class TreeCanvas;
class QStandardItemModel;

class MyProxyModel: public QSortFilterProxyModel
{
public:
  MyProxyModel(QWidget* parent) : QSortFilterProxyModel(parent) {}

protected:
  bool lessThan ( const QModelIndex & left, const QModelIndex & right ) const {
    if (left.column() == 0) {
      int lhs = sourceModel()->data(sourceModel()->index(left.row(), 0)).toInt();
      int rhs = sourceModel()->data(sourceModel()->index(right.row(), 0)).toInt();
      return lhs < rhs;
    } else if (left.column() == 1) {
      int lhs = sourceModel()->data(sourceModel()->index(left.row(), 1)).toString().size();
      int rhs = sourceModel()->data(sourceModel()->index(right.row(), 1)).toString().size();
      return lhs < rhs;
    }
  }
};

class NogoodDialog : public QDialog {
  Q_OBJECT

private:

  static const int DEFAULT_WIDTH;
  static const int DEFAULT_HEIGHT;

  TreeCanvas& _tc;

  // QTableWidget* _nogoodTable;
  QTableView* _nogoodTable;
  const std::unordered_map<unsigned long long, std::string>& _sid2nogood;

  QStandardItemModel* _model;
  MyProxyModel* _proxy_model;

private:

  void populateTable();

private Q_SLOTS:

  void selectNode(const QModelIndex & index);

public:

  NogoodDialog(QWidget* parent, TreeCanvas& tc,
    const std::unordered_map<unsigned long long, std::string>& sid2nogood);

  ~NogoodDialog();


};



#endif
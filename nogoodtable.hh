#ifndef NOGOODTABLE_H
#define NOGOODTABLE_H

#include <QTableView>
#include <qsortfilterproxymodel.h>
#include <qboxlayout.h>
#include <qpushbutton.h>

#include "execution.hh"
#include "treecanvas.hh"

class NogoodProxyModel : public QSortFilterProxyModel {
public:
  enum Sorter {SORTER_INT, SORTER_NOGOOD};
  explicit NogoodProxyModel(QWidget* parent, const QList<Sorter>& sorters);

  bool enabled {true};
protected:
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

private:
  QList<Sorter> _sorters;
};

class ReasonLocationFilterProxyModel : public QSortFilterProxyModel {
public:
  ReasonLocationFilterProxyModel(const Execution& e,
                                 int sid_col);

  virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
  void setReasonLocationFilter(const QList<QList<int> >& location);

private:
  const int _sid_col;
  const Execution& _execution;
  QList<QList<int> > _locationFilter;
};

class NogoodTableView : public QTableView {
public:
  NogoodTableView(QWidget* parent,
              QStandardItemModel* model,
              QList<NogoodProxyModel::Sorter> sorters,
              int sid_col, int nogood_col);

  NogoodProxyModel* getProxyModel();

  void connectHeatmapButton(const QPushButton* heatmapButton,
                            const Execution& e,
                            const TreeCanvas& tc) const;
  void connectShowExpressionsButton(const QPushButton* showExpressions, const Execution& e);
  void connectFilters(const QLineEdit* regex_edit);

private:
  QStandardItemModel* _model;
  NogoodProxyModel* nogood_proxy_model;
  int _sid_col;
  int _nogood_col;
  bool expand_expressions;
};

#endif // NOGOODTABLE_H

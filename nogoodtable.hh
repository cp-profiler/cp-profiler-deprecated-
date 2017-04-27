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
  enum Sorter {SORTER_SID = 0, SORTER_INT, SORTER_NOGOOD};
  explicit NogoodProxyModel(QWidget* parent,
                            const Execution& e,
                            const QList<Sorter>& sorters);

  void setTextFilterStrings(const QStringList& textFilterStrings);
  void emptyTextFilterStrings();
  void setLocationFilter(const LocationFilter& locationFilter);

protected:
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
  bool filterAcceptsRow(int source_row, const QModelIndex&) const;

private:
  QList<Sorter> _sorters;
  QStringList _text_filter;
  LocationFilter _loc_filter;
  const std::unordered_map<int64_t, std::string*>& _sid2info;
  const NameMap* _nm;
  int _sid_col;
  int _nogood_col;
};

class NogoodTableView : public QTableView {
public:
  NogoodTableView(QWidget* parent,
                  QStandardItemModel* model,
                  QList<NogoodProxyModel::Sorter> sorters,
                  const Execution& e,
                  int sid_col, int nogood_col);

  void connectHeatmapButton(const QPushButton* heatmapButton, const TreeCanvas& tc) const;
  void connectShowExpressionsButton(const QPushButton* showExpressions);
  void connectTextFilter(const QLineEdit* text_edit);
  void connectLocationFilter(const QLineEdit* location_edit);
  void connectLocationButton(const QPushButton* locationButton,
                             QLineEdit* location_edit) const;
private:
  QModelIndexList getSelection() const;
  int64_t getSidFromRow(int row) const;
  void updateSelection() const;
  void updateLocationFilter(QLineEdit* location_edit) const;

  const Execution& _execution;
  QStandardItemModel* _model;
  NogoodProxyModel* nogood_proxy_model;
  int _sid_col;
  int _nogood_col;
  bool expand_expressions;

private slots:
  void refreshModelRenaming();
  const QString getHeatMapFromModel() const;
};

#endif // NOGOODTABLE_H

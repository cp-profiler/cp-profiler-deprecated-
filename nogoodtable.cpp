
#include <cassert>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <QDialog>
#include <qobject.h>

#include "nogoodtable.hh"
#include "treecanvas.hh"

NogoodProxyModel::NogoodProxyModel(QWidget* parent, const QList<Sorter>& sorters)
    : QSortFilterProxyModel(parent), _sorters(sorters) {}

bool NogoodProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
  int col = left.column();
  switch(_sorters[col]) {
  case SORTER_INT:
  {
    int lhs = sourceModel()->data(sourceModel()->index(left.row(), col)).toInt();
    int rhs = sourceModel()->data(sourceModel()->index(right.row(), col)).toInt();
    return lhs < rhs;
  }
  case SORTER_NOGOOD:
  {
    const QString& lhs = sourceModel()->data(sourceModel()->index(left.row(), col)).toString();
    const QString& rhs = sourceModel()->data(sourceModel()->index(right.row(), col)).toString();
    return (lhs.size() < rhs.size()) || (lhs < rhs);
  }
  }
  assert(false);
  return false;  /// should not reach here; to prevent a warning
}

ReasonLocationFilterProxyModel::ReasonLocationFilterProxyModel(
    const Execution& e, int sid_col)
  : _sid_col(sid_col), _execution(e) {}

bool ReasonLocationFilterProxyModel::filterAcceptsRow(
    int source_row, const QModelIndex &source_parent) const {
  const NameMap* nm = _execution.getNameMap();
  const QModelIndex index = mapFromSource(source_parent);
  int64_t sid = index.sibling(index.row(), _sid_col).data().toLongLong();
  for(int reason : getReasons(sid, _execution.getInfo())) {
    const QString& path = nm->getPath(QString::number(reason));
  }
}

void ReasonLocationFilterProxyModel::setReasonLocationFilter(
    const QList<QList<int> >& location) {
  _locationFilter = location;
}


NogoodTableView::NogoodTableView(QWidget* parent,
                                 QStandardItemModel* model,
                                 QList<NogoodProxyModel::Sorter> sorters,
                                 int sid_col, int nogood_col)
  : QTableView(parent), _model(model),
    _sid_col(sid_col), _nogood_col(nogood_col),
    expand_expressions(false) {
  horizontalHeader()->setStretchLastSection(true);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  resizeColumnsToContents();

  nogood_proxy_model = new NogoodProxyModel(this, sorters);
  nogood_proxy_model->setSourceModel(_model);

  setModel(nogood_proxy_model);
  setSortingEnabled(true);
}

void NogoodTableView::connectHeatmapButton(const QPushButton* heatmapButton,
                                           const Execution& e,
                                           const TreeCanvas& tc) const {
  connect(heatmapButton, &QPushButton::clicked, [&tc,this,&e](){
    const QString heatmap = e.getNameMap()->getHeatMapFromModel(
          e.getInfo(), *this, _sid_col);
    if(!heatmap.isEmpty())
      tc.emitShowNogoodToIDE(heatmap);
  });
}

void NogoodTableView::connectShowExpressionsButton(
    const QPushButton* showExpressions,
    const Execution& e) {
  connect(showExpressions, &QPushButton::clicked, [&e,this](){
    expand_expressions ^= true;
    setSortingEnabled(false);
    e.getNameMap()->refreshModelRenaming(
          e.getNogoods(),
          *this, *_model, *nogood_proxy_model,
          _sid_col, _nogood_col, expand_expressions);
    setSortingEnabled(true);
  });
}

void NogoodTableView::connectFilters(const QLineEdit* regex_edit) {
  connect(regex_edit, &QLineEdit::returnPressed, [this, regex_edit] () {
    const QStringList splitFilter = regex_edit->text().split(",");
    const QRegExp reg_filter("^(?=.*" + splitFilter.join(")(?=.*") + ").*$");
    nogood_proxy_model->setFilterKeyColumn(_nogood_col);
    nogood_proxy_model->setFilterRegExp(reg_filter);
  });
}

NogoodProxyModel* NogoodTableView::getProxyModel() {
  return nogood_proxy_model;
}

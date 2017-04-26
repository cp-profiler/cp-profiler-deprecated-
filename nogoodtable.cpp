
#include <cassert>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <QDialog>
#include <qobject.h>

#include "nogoodtable.hh"
#include "treecanvas.hh"

NogoodProxyModel::NogoodProxyModel(QWidget* parent,
                                   const Execution& e,
                                   const QList<Sorter>& sorters)
    : QSortFilterProxyModel(parent), _sorters(sorters),
      _sid2info(e.getInfo()), _nm(e.getNameMap()) {
  _sid_col = sorters.indexOf(NogoodProxyModel::SORTER_SID);
  _nogood_col = sorters.indexOf(NogoodProxyModel::SORTER_NOGOOD);
}

void NogoodProxyModel::setLocationFilter(const LocationFilter& locationFilter) {
  _loc_filter = locationFilter;
  invalidateFilter();
}

void NogoodProxyModel::emptyTextFilterStrings() {
  _text_filter.clear();
}

void NogoodProxyModel::setTextFilterStrings(const QStringList& textFilterStrings) {
  _text_filter = textFilterStrings;
  invalidateFilter();
}

bool NogoodProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
  int col = left.column();
  switch(_sorters[col]) {
  case SORTER_SID:
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

bool NogoodProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const {
  const QString& nogood = sourceModel()->data(
              sourceModel()->index(source_row, _nogood_col)).toString();

  bool text_matches = std::all_of(
              _text_filter.begin(), _text_filter.end(),
              [&nogood](const QString& tf) { return nogood.contains(tf); });

  const int sid = sourceModel()->data(sourceModel()->index(source_row, _sid_col)).toInt();
  auto reasons = getReasons(sid, _sid2info);
  bool loc_matches = true;
  if(_nm) {
    loc_matches = std::all_of(reasons.begin(),
                              reasons.end(),
                              [this](const int cid) {
      return _loc_filter.contains(_nm->getLocation(QString::number(cid)));
    });
  }

  return text_matches && loc_matches;
}

NogoodTableView::NogoodTableView(QWidget* parent,
                                 QStandardItemModel* model,
                                 QList<NogoodProxyModel::Sorter> sorters,
                                 const Execution& e,
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

  nogood_proxy_model = new NogoodProxyModel(this, e, sorters);
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

void NogoodTableView::connectLocationButton(const QPushButton* locationButton,
                                            QLineEdit* location_edit,
                                            const Execution& e) {
  connect(locationButton, &QPushButton::clicked, [&e,this,location_edit](){
    e.getNameMap()->updateLocationFilter(e.getInfo(), *this, location_edit, _sid_col);
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

void NogoodTableView::connectTextFilter(const QLineEdit* text_edit) {
  connect(text_edit, &QLineEdit::returnPressed, [this, text_edit] () {
    const QStringList splitFilter = text_edit->text().split(",");
    nogood_proxy_model->setTextFilterStrings(splitFilter);
  });
}

void NogoodTableView::connectLocationFilter(const QLineEdit* location_edit) {
  connect(location_edit, &QLineEdit::returnPressed, [this, location_edit] () {
    nogood_proxy_model->setLocationFilter(LocationFilter::fromString(location_edit->text()));
  });
}

NogoodProxyModel* NogoodTableView::getProxyModel() {
  return nogood_proxy_model;
}

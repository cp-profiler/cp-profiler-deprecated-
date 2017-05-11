
#include <cassert>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qdialog.h>

#include "cpprofiler/utils/nogood_subsumption.hh"
#include "nogoodtable.hh"
#include "treecanvas.hh"
#include "execution.hh"


// NogoodProxyModel
// =============================================================
NogoodProxyModel::NogoodProxyModel(QWidget* parent,
                                   const Execution& e,
                                   const QVector<Sorter>& sorters)
    : QSortFilterProxyModel(parent), _sorters(sorters),
      _sid2info(e.getInfo()), _nm(e.getNameMap()) {
  _sid_col = sorters.indexOf(NogoodProxyModel::SORTER_SID);
  _nogood_col = sorters.indexOf(NogoodProxyModel::SORTER_NOGOOD);
}

void NogoodProxyModel::setLocationFilter(const LocationFilter& locationFilter) {
  _loc_filter = locationFilter;
  invalidateFilter();
}

void NogoodProxyModel::setTextFilterStrings(const QStringList& includeTextFilter,
                                            const QStringList& rejectTextFilter) {
  _include_text_filter = includeTextFilter;
  _reject_text_filter = rejectTextFilter;
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
              _include_text_filter.begin(), _include_text_filter.end(),
              [&nogood](const QString& tf) { return nogood.contains(tf); });
  text_matches &= std::none_of(
              _reject_text_filter.begin(), _reject_text_filter.end(),
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

// NogoodTableView
// =============================================================

NogoodTableView::NogoodTableView(QWidget* parent,
                                 QStandardItemModel* model,
                                 QVector<NogoodProxyModel::Sorter> sorters,
                                 const Execution& e,
                                 int sid_col, int nogood_col)
  : QTableView(parent), _execution(e), _model(model),
    _sid_col(sid_col), _nogood_col(nogood_col), expand_expressions(false) {
  horizontalHeader()->setStretchLastSection(true);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  resizeColumnsToContents();

  nogood_proxy_model = new NogoodProxyModel(this, _execution, sorters);
  nogood_proxy_model->setSourceModel(_model);

  setModel(nogood_proxy_model);
  setSortingEnabled(true);
}

int64_t NogoodTableView::getSidFromRow(int row) const {
  QModelIndex proxy_index = nogood_proxy_model->index(row, _sid_col, QModelIndex());
  QModelIndex mapped_index = nogood_proxy_model->mapToSource(proxy_index);
  return _model->data(mapped_index).toLongLong();
}

void NogoodTableView::getHeatmapAndEmit(const TreeCanvas& tc, bool record = false) const {
  const QModelIndexList selection = getSelection();
  QStringList label;
  std::unordered_map<int, int> con_ids;

  int max_count = 0;
  for(int i=0; i<selection.count(); i++) {
    int64_t sid = getSidFromRow(selection.at(i).row());
    for(int con_id : getReasons(sid, _execution.getInfo())) {
      int count = 0;
      if(con_ids.find(con_id) == con_ids.end()) {
        con_ids[con_id] = 1;
        count = 1;
      } else {
        con_ids[con_id]++;
        count = con_ids[con_id];
      }
      max_count = count > max_count ? count : max_count;
    }

    label << QString::number(sid);
  }

  updateSelection();

  QString heatMap = _execution.getNameMap()->getHeatMap(con_ids, max_count);
  QString text = label.join(" ");
  if(!heatMap.isEmpty())
    tc.emitShowNogoodToIDE(heatMap, text, record);
}

void NogoodTableView::connectHeatmapButton(const QPushButton* heatmapButton,
                                           const TreeCanvas& tc) {
  connect(this, &NogoodTableView::clicked,
          [&tc, this](){getHeatmapAndEmit(tc, false);});
  connect(heatmapButton, &QPushButton::clicked,
          [&tc, this](){getHeatmapAndEmit(tc, true);});
}

void NogoodTableView::connectLocationButton(const QPushButton* locationButton,
                                            QLineEdit* location_edit) {
  connect(locationButton, &QPushButton::clicked, [this,location_edit](){
    updateLocationFilter(location_edit);
  });
}

QModelIndexList NogoodTableView::getSelection() const {
  QModelIndexList selection = selectionModel()->selectedRows();
  if(selection.count() == 0)
    for(int row=0; row<model()->rowCount(); row++)
      selection.append(model()->index(row, _sid_col));
  return selection;
}

void NogoodTableView::updateSelection() const {
  QItemSelection selection = selectionModel()->selection();
  selectionModel()->select(
        selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void NogoodTableView::refreshModelRenaming() {
  expand_expressions ^= true;
  setSortingEnabled(false);
  const QModelIndexList selection = getSelection();
  for(int i=0; i<selection.count(); i++) {
    int row = selection.at(i).row();
    int64_t sid = getSidFromRow(row);

    const QString& qclause = QString::fromStdString(_execution.getNogoodBySid(sid));
    const NameMap* nm = _execution.getNameMap();
    if(!qclause.isEmpty() && nm) {
      const QString clause = nm->replaceNames(qclause, expand_expressions);
      QModelIndex idx = nogood_proxy_model->mapToSource(nogood_proxy_model->index(row, 0, QModelIndex()));
      _model->setData(idx.sibling(idx.row(), _nogood_col), clause);
    }
  }
  updateSelection();
  setSortingEnabled(true);
}

void NogoodTableView::connectShowExpressionsButton(const QPushButton* showExpressions) {
  connect(showExpressions, &QPushButton::clicked, this, &NogoodTableView::refreshModelRenaming);
}


QString negateLit(const QString& lit) {
  static QRegExp op_rx("(<=)|(>=)|(==)|(!=)|(=)|(<)|(>)");
  static const QHash<QString, QString> neg
  {
      {"<=", ">"},
      {">=", "<"},
      {"==", "!="},
      {"=", "!="},
      {"!=", "=="},
      {"<", ">="},
      {">", "<="}
  };

  long pos = op_rx.indexIn(lit);
  QString negLit = lit.left(pos) + lit.mid(pos, op_rx.matchedLength()) + lit.right(lit.size()-op_rx.matchedLength());
  return negLit;
}

void NogoodTableView::renameSubsumedSelection() {
  setSortingEnabled(false);

  std::vector<int64_t> pool;
  for(int row=0; row<nogood_proxy_model->rowCount(); row++)
    pool.push_back(getSidFromRow(row));
  Utils::SubsumptionFinder sf(_execution, pool);

  QVector<int64_t> sids;
  const QModelIndexList selection = getSelection();
  for(int i=0; i<selection.count(); i++) {
    int row = selection.at(i).row();
    int64_t isid = getSidFromRow(row);

    QString finalString = sf.getSubsumingClauseString(isid);
    const NameMap* nm = _execution.getNameMap();
    if(nm)
      finalString = _execution.getNameMap()->replaceNames(finalString, expand_expressions);

    QModelIndex idx = nogood_proxy_model->mapToSource(nogood_proxy_model->index(row, 0, QModelIndex()));
    _model->setData(idx.sibling(idx.row(), _nogood_col), finalString);
  }

  updateSelection();
  setSortingEnabled(true);
}

void NogoodTableView::connectSubsumButton(const QPushButton* subsumButton) {
  connect(subsumButton, &QPushButton::clicked,
         this, &NogoodTableView::renameSubsumedSelection);
  //connect(subsumButton, &QPushButton::clicked,
  //        this, &NogoodTableView::renameResolvingSubsumption);
}

QString convertToFlatZinc(const QString& clause) {

  QRegExp op_rx("(<=)|(>=)|(==)|(!=)|(=)|(<)|(>)");
  QHash<QString, QString> neg
  {
      {"<=", ">"},
      {">=", "<"},
      {"==", "!="},
      {"=", "!="},
      {"!=", "=="},
      {"<", ">="},
      {">", "<="}
  };
  QStringList fzn;

  const QStringList literals = clause.split(" ");
  for(const QString& lit : literals) {
    if(!lit.isEmpty()) {
      int pos = lit.indexOf(op_rx);
      QString left = lit.left(pos);
      QString op = lit.mid(pos, op_rx.matchedLength());
      QString right = lit.right(lit.size() - (pos + op_rx.matchedLength()));
      //std::cerr << "left: " << left.toStdString() << " op: " << op.toStdString() << " right: " << right.toStdString() << "\n";
      if(op == "==" || op == "=") {
        op = "int_eq";
      } else if (op == "!=") {
        op = "int_ne";
      } else if (op == "<=") {
        op = "int_le";
      } else if (op == "<") {
        op = "int_lt";
      } else if (op == ">=") {
        op = "int_ge";
      } else if (op == ">") {
        op = "int_gt";
      }

      fzn << QString("constraint %0(%1,%2);").arg(op).arg(left).arg(right);
    }
  }

  return fzn.join("\n");
}

void NogoodTableView::showFlatZinc() {
  setSortingEnabled(false);
  const QModelIndexList selection = getSelection();
  for(int i=0; i<selection.count(); i++) {
    int row = selection.at(i).row();
    int64_t sid = getSidFromRow(row);

    const QString& qclause = QString::fromStdString(_execution.getNogoodBySid(sid));
    if(!qclause.isEmpty()) {
      const QString clause = convertToFlatZinc(qclause);
      QModelIndex idx = nogood_proxy_model->mapToSource(nogood_proxy_model->index(row, 0, QModelIndex()));
      _model->setData(idx.sibling(idx.row(), _nogood_col), clause);
    }
  }
  updateSelection();
  setSortingEnabled(true);
}

void NogoodTableView::connectFlatZincButton(const QPushButton* getFlatZinc) {
  connect(getFlatZinc, &QPushButton::clicked, this, &NogoodTableView::showFlatZinc);
}

void NogoodTableView::connectTextFilter(const QLineEdit* include_edit,
                                        const QLineEdit* reject_edit) {
  auto setFilters = [this, include_edit, reject_edit] () {
    const QStringList includeSplitFilter = include_edit->text().split(",");
    const QStringList rejectSplitFilter = reject_edit->text().split(",");
    nogood_proxy_model->setTextFilterStrings(includeSplitFilter, rejectSplitFilter);
  };
  connect(include_edit, &QLineEdit::returnPressed, setFilters);
  connect(reject_edit, &QLineEdit::returnPressed, setFilters);
}

void NogoodTableView::updateLocationFilter(QLineEdit* location_edit) const {
  QStringList locationFilterText;
  const QModelIndexList selection = getSelection();
  for(int i=0; i<selection.count(); i++) {
    int64_t sid = getSidFromRow(selection.at(i).row());
    auto reasons = getReasons(sid, _execution.getInfo());
    locationFilterText << _execution.getNameMap()->getLocationFilterString(reasons);
  }

  location_edit->setText(LocationFilter::fromString(locationFilterText.join(",")).toString());
}

void NogoodTableView::connectLocationFilter(QLineEdit* location_edit) {
  connect(location_edit, &QLineEdit::returnPressed, [this, location_edit] () {
    LocationFilter lf = LocationFilter::fromString(location_edit->text());
    location_edit->setText(lf.toString());
    nogood_proxy_model->setLocationFilter(lf);
  });
}


#include "nogoodtable.hh"
#include "treecanvas.hh"
#include "execution.hh"
#include "cpprofiler/utils/nogood_subsumption.hh"

using std::string;

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
  return false;  /// should not reach here; to prevent a warning
}

bool NogoodProxyModel::filterAcceptsText(const QString& nogood) const {
  bool text_matches = std::all_of(
              _include_text_filter.begin(), _include_text_filter.end(),
              [&nogood](const QString& tf) { return nogood.contains(tf); });
  text_matches &= std::none_of(
              _reject_text_filter.begin(), _reject_text_filter.end(),
              [&nogood](const QString& tf) { return !tf.isEmpty() && nogood.contains(tf); });
  return text_matches;
}

bool NogoodProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const {
  const QString& nogood = sourceModel()->data(
              sourceModel()->index(source_row, _nogood_col)).toString();

  bool text_matches = filterAcceptsText(nogood);

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

    const string& qclause = _execution.getNogoodBySid(static_cast<int>(sid));
    if(!qclause.empty()) {
      QModelIndex idx = nogood_proxy_model->mapToSource(nogood_proxy_model->index(row, 0, QModelIndex()));
      _model->setData(idx.sibling(idx.row(), _nogood_col), QString::fromStdString(qclause));
    }
  }
  updateSelection();
  setSortingEnabled(true);
}

void NogoodTableView::connectShowExpressionsButton(const QPushButton* showExpressions) {
  connect(showExpressions, &QPushButton::clicked, this, &NogoodTableView::refreshModelRenaming);
}

void NogoodTableView::renameSubsumedSelection(const QCheckBox* useAll,
                                              const QCheckBox* applyFilter) {
  setSortingEnabled(false);

  std::vector<int64_t> pool;
  if(useAll->isChecked()) {

    auto& sid2nogood = _execution.getNogoods();
    for(auto& sidNogood : sid2nogood) {

      if(sidNogood.second.empty()) continue;

      int64_t sid = sidNogood.first;
      const auto nogood = QString::fromStdString(sidNogood.second);

      if(!applyFilter->isChecked() || nogood_proxy_model->filterAcceptsText(nogood)) {
        pool.push_back(sid);
      }

    }
  } else {
    for(int row=0; row<nogood_proxy_model->rowCount(); row++) {
      pool.push_back(getSidFromRow(row));
    }
  }

  utils::SubsumptionFinder sf(_execution.getNogoods(), pool);

  const QModelIndexList selection = getSelection();
  for(int i=0; i<selection.count(); i++) {
    int row = selection.at(i).row();
    int64_t isid = getSidFromRow(row);

    const string finalString = sf.getSubsumingClauseString(isid);
    QModelIndex idx = nogood_proxy_model->mapToSource(nogood_proxy_model->index(row, 0, QModelIndex()));
    _model->setData(idx.sibling(idx.row(), _nogood_col), QString::fromStdString(finalString));
  }

  updateSelection();
  setSortingEnabled(true);
}

void NogoodTableView::connectSubsumButtons(const QPushButton* subsumButton,
                                           const QCheckBox* useAll,
                                           const QCheckBox* applyFilter) {
  connect(subsumButton, &QPushButton::clicked,
          this, [subsumButton, useAll, applyFilter, this](){
      NogoodTableView::renameSubsumedSelection(useAll, applyFilter);
  });
}

string convertToFlatZinc(const string& clause) {

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

  const QStringList literals = QString::fromStdString(clause).split(" ");
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

  return fzn.join("\n").toStdString();
}

void NogoodTableView::showFlatZinc() {
  setSortingEnabled(false);
  const QModelIndexList selection = getSelection();
  for(int i=0; i<selection.count(); i++) {
    int row = selection.at(i).row();
    int64_t sid = getSidFromRow(row);

    const string clause = _execution.getNogoodBySid(static_cast<int>(sid));
    if(!clause.empty()) {
      const string fzn = convertToFlatZinc(clause);
      QModelIndex idx = nogood_proxy_model->mapToSource(nogood_proxy_model->index(row, 0, QModelIndex()));
      _model->setData(idx.sibling(idx.row(), _nogood_col), QString::fromStdString(fzn));
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

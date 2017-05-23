
#include "nogoodtable.hh"
#include "treecanvas.hh"
#include "execution.hh"
#include "cpprofiler/utils/nogood_subsumption.hh"
#include "cpprofiler/utils/string_utils.hh"

using std::string;
using std::vector;

QString lit2string(utils::lits::Lit l) {
    QStringList sl;
    sl << QString::fromStdString(l.var)
       << QString::fromStdString(l.op)
       << QString::number(l.val);
    return sl.join("");
}

// NogoodDelegate
// =============================================================
class NogoodDelegate : public QStyledItemDelegate {
public:
    NogoodDelegate(std::unordered_map<std::string, QColor>& colors,
                   int nogood_col) : _colors(colors), _nogood_col(nogood_col) {}

protected:
  void paint(QPainter* painter, const QStyleOptionViewItem& item, const QModelIndex& index) const {
    QStyleOptionViewItem itemCpy = item;
    initStyleOption(&itemCpy, index);

    QStyle *style = itemCpy.widget? itemCpy.widget->style() : QApplication::style();

    QTextDocument doc;

    if(index.column() == _nogood_col && itemCpy.text.left(10) != "constraint") {
      QStringList litsHtml;
      const QString& clause = itemCpy.text;
      for(QString& ls : clause.split(" ", QString::SplitBehavior::SkipEmptyParts)) {
        utils::lits::Lit lit = utils::lits::parse_lit(ls.toStdString());
        QStringList litstring;
        QColor c = _colors.at(lit.var);
        litstring << "<span style=\"color:" << c.name()  << ";\">" << lit2string(lit).toHtmlEscaped() << "</span>";
        litsHtml.append(litstring.join(""));
      }
      doc.setHtml(litsHtml.join(" "));
    } else {
      QStringList newText;
      for(QString& s : itemCpy.text.split(";", QString::SplitBehavior::SkipEmptyParts))
        newText << s.toHtmlEscaped();
      doc.setHtml(newText.join("<br/>"));
    }

    // Painting item without text
    itemCpy.text = QString();
    style->drawControl(QStyle::CE_ItemViewItem, &itemCpy, painter);

    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &itemCpy);
    painter->save();
    painter->translate(textRect.topLeft());
    painter->setClipRect(textRect.translated(-textRect.topLeft()));
    doc.documentLayout()->draw(painter, QAbstractTextDocumentLayout::PaintContext());
    painter->restore();
  }
private:
  std::unordered_map<std::string, QColor>& _colors;
  int _nogood_col;
};

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
      return _loc_filter.contains(_nm->getLocation(std::to_string(cid)));
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

  auto& sid2nogood = _execution.getNogoods();
  for(auto& sidNogood : sid2nogood) {
     vector<string> clause = utils::split(sidNogood.second, ' ');
     for(string s : clause) {
         utils::lits::Lit l = utils::lits::parse_lit(s);
         if(_colors.find(l.var) == _colors.end()) {
             int r=255,g=255,b=255;
             while(r>200 && g>200 && b>200) {
                 r = rand()%255;
                 g = rand()%255;
                 b = rand()%255;
             }
             _colors[l.var] = QColor(r,g,b);
         }
     }
  }
  setItemDelegate(new NogoodDelegate(_colors, _nogood_col));
}

int64_t NogoodTableView::getSidFromRow(int row) const {
  QModelIndex proxy_index = nogood_proxy_model->index(row, _sid_col, QModelIndex());
  QModelIndex mapped_index = nogood_proxy_model->mapToSource(proxy_index);
  return _model->data(mapped_index).toLongLong();
}

void NogoodTableView::getHeatmapAndEmit(const TreeCanvas& tc, bool record = false) const {
  const QModelIndexList selection = getSelection();
  vector<string> label;
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

    label.push_back(std::to_string(sid));
  }

  updateSelection();

  std::string heatMap = _execution.getNameMap()->getHeatMap(con_ids, max_count);
  std::string text = utils::join(label, ' ');
  if(!heatMap.empty())
    tc.emitShowNogoodToIDE(QString::fromStdString(heatMap),
                           QString::fromStdString(text),
                           record);
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

void NogoodTableView::renameSubsumedSelection(const QCheckBox* resolution,
                                              const QCheckBox* use_all,
                                              const QCheckBox* apply_filter,
                                              const QCheckBox* only_earlier_sids) {
  setSortingEnabled(false);

  std::vector<int64_t> pool;
  if(use_all->isChecked()) {

    auto& sid2nogood = _execution.getNogoods();
    for(auto& sidNogood : sid2nogood) {

      if(sidNogood.second.empty()) continue;

      int64_t sid = sidNogood.first;
      const auto nogood = QString::fromStdString(sidNogood.second);

      if(!apply_filter->isChecked() || nogood_proxy_model->filterAcceptsText(nogood)) {
        pool.push_back(sid);
      }

    }
  } else {
    for(int row=0; row<nogood_proxy_model->rowCount(); row++) {
      pool.push_back(getSidFromRow(row));
    }
  }

  utils::subsum::SubsumptionFinder sf(_execution.getNogoods(), pool);

  const QModelIndexList selection = getSelection();
  for(int i=0; i<selection.count(); i++) {
    int row = selection.at(i).row();
    int64_t isid = getSidFromRow(row);

    string finalString;
    if(resolution->isChecked())
      finalString = sf.getSelfSubsumingResolutionString(isid, only_earlier_sids->isChecked());
    else
      finalString = sf.getSubsumingClauseString(isid, only_earlier_sids->isChecked());
    QModelIndex idx = nogood_proxy_model->mapToSource(nogood_proxy_model->index(row, 0, QModelIndex()));
    _model->setData(idx.sibling(idx.row(), _nogood_col), QString::fromStdString(finalString));
  }

  updateSelection();
  setSortingEnabled(true);
}

void NogoodTableView::connectSubsumButtons(const QPushButton* subsumButton,
                                           const QCheckBox* resolution,
                                           const QCheckBox* use_all,
                                           const QCheckBox* apply_filter,
                                           const QCheckBox* only_earlier_sids) {
  connect(subsumButton, &QPushButton::clicked,
          this, [subsumButton, resolution, use_all,
                 apply_filter, only_earlier_sids, this](){
      NogoodTableView::renameSubsumedSelection(resolution, use_all,
                                               apply_filter, only_earlier_sids);
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
  vector<string> locationFilterText;
  const QModelIndexList selection = getSelection();
  for(int i=0; i<selection.count(); i++) {
    int64_t sid = getSidFromRow(selection.at(i).row());
    auto reasons = getReasons(sid, _execution.getInfo());
    locationFilterText.push_back(_execution.getNameMap()->getLocationFilterString(reasons));
  }

  location_edit->setText(QString::fromStdString(
                             LocationFilter::fromString(utils::join(locationFilterText, ',')).toString()));
}

void NogoodTableView::connectLocationFilter(QLineEdit* location_edit) {
  connect(location_edit, &QLineEdit::returnPressed, [this, location_edit] () {
    LocationFilter lf = LocationFilter::fromString(location_edit->text().toStdString());
    location_edit->setText(QString::fromStdString(lf.toString()));
    nogood_proxy_model->setLocationFilter(lf);
  });
}

#include "namemap.hh"

#include <qstringlist.h>
#include <qtextedit.h>
#include <math.h>
#include <qtextstream.h>
#include <qvector.h>

#include "third-party/json.hpp"

#define reg_mzn_ident "[A-Za-z][A-Za-z0-9_]*"
#define reg_number "[0-9]*"

QRegExp NameMap::var_name_regex(reg_mzn_ident);
QRegExp NameMap::assignment_regex(reg_mzn_ident "=" reg_number);

NameMap::NameMap(SymbolTable& st) : _nameMap(st) {};

NameMap::NameMap(QString& path_filename, QString& model_filename) {
  std::vector<QString> modelText;
  QFile model_file(model_filename);
  if(model_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString line;
    QTextStream in(&model_file);
    while (!in.atEnd()) {
      line = in.readLine();
      modelText.push_back(line);
    }
  }

  QFile pf(path_filename);
  if(pf.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString line;
    QTextStream in(&pf);
    while(!in.atEnd()) {
      line = in.readLine();
      QStringList s = line.split("\t");
      _nameMap[s[0]] = std::make_pair(s[1], s[2]);
      if(s[1].left(12) == "X_INTRODUCED") {
        addIdExpressionToMap(s[0], modelText);
      }
    }
  }
}

const QString NameMap::getNiceName(const QString& ident) const {
  auto it = _nameMap.find(ident);
  if(it != _nameMap.end()) {
    return it.value().first;
  }
  return "";
}

const QString NameMap::getPath(const QString& ident) const {
  auto it = _nameMap.find(ident);
  if(it != _nameMap.end()) {
    return it.value().second;
  }
  return "";
}

const QString NameMap::replaceNames(const QString& text, bool expand_expressions) const {
  if (_nameMap.size() == 0) {
    return text;
  }

  QStringList ss;
  long pos = 0;
  long prev = 0;
  while((pos = var_name_regex.indexIn(text, prev)) != -1) {
    ss += text.mid(prev, pos-prev);
    const Ident& id = text.mid(pos, var_name_regex.matchedLength());
    QString name = getNiceName(id);
    if(expand_expressions && name.left(12) == "X_INTRODUCED") {
      auto eit = _expressionMap.find(name);
      if(eit != _expressionMap.end()) {
        name = "\'" + *eit + "\'";
      } else {
        name = id;
      }
    }
    ss += (name != "" ? name : id);
    prev = pos + var_name_regex.matchedLength();
  }
  ss << text.mid(prev, text.size());
  return ss.join("");
}

QString NameMap::replaceAssignments(const QString& path, const QString& expression) const {
    NameMap::SymbolTable st;

    (void) assignment_regex.indexIn(path);
    const QStringList assignments = assignment_regex.capturedTexts();
    for(const QString& assign : assignments) {
      QStringList leftright = assign.split("=");
      st[leftright[0]] = std::make_pair(leftright[1], "");
    }

    NameMap nm(st);
    return nm.replaceNames(expression);
}

void NameMap::addIdExpressionToMap(const Ident& ident, const std::vector<QString>& modelText) {
  if(modelText.size() == 0) return;
  const QString& path = _nameMap[ident].second;
  const QStringList components = getPathHead(path, true);

  QStringList locList = components.last().split(":");
  bool ok;
  int sl = locList[1].toInt(&ok);
  int sc = locList[2].toInt(&ok);
  // Ignore end-line for now
  //int el = locList[3].toInt(&ok);
  int ec = locList[4].toInt(&ok);

  // Extract text and store it in map
  QString expression = modelText[sl-1].mid(sc-1, ec-(sc-1));
  expression = replaceAssignments(components.join(";"), expression);

  _expressionMap.insert(ident, expression);
}

// Get the most specific path component that is still in the user model
const QStringList NameMap::getPathHead(const QString& path, bool includeTrail = false) const {
  QStringList pathSplit = path.split(";");

  QString mzn_file;
  QStringList previousHead;
  int i=0;
  do {
    QString path_head = pathSplit[i];
    QStringList head = path_head.split(":");
    QString head_file;
    if(head.size() > 0) {
      if(i==0) mzn_file = head[0];
      head_file = head[0];
    }

    if(head_file != mzn_file)
      return previousHead;

    if(!includeTrail)
      previousHead.clear();
    previousHead << path_head;
    i++;
  } while(i < pathSplit.size());

  return previousHead;
}

const QString NameMap::getHeatMap(
    const std::unordered_map<int, int>& con_id_counts,
    int max_count,
    const QString& desc) const {
  int bucket = int(ceil(255.0/double(max_count+1)));
  QStringList highlight_url;
  highlight_url << "<a href=\"highlight://?";
  for(auto it : con_id_counts) {
    const QString con_string = QString::number(it.first);
    const QString path = getPath(con_string);
    const QString path_head = getPathHead(path, false)[0];
    QStringList location_etc = path_head.split(":");
    int count = it.second;

    if(location_etc.count() >= 5) {
      QStringList newLoc;
      for(int i=0; i<5; i++) newLoc << location_etc[i];
      int val = (1 + count) * bucket;
      newLoc << QString::number(val <= 255 ? val : 255);
      highlight_url << newLoc.join(":") << ";";
    }
  }
  highlight_url << "\">Heatmap ("<< desc << ")</a>";

  return highlight_url.join("");
}

namespace TableHelpers {
  void updateSelection(const QTableView& table) {
    QItemSelection selection = table.selectionModel()->selection();
    table.selectionModel()->select(
          selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  QModelIndexList getSelection(const QTableView& table, int sid_col) {
    QModelIndexList selection = table.selectionModel()->selectedRows();
    if(selection.count() == 0)
      for(int row=0; row<table.model()->rowCount(); row++)
        selection.append(table.model()->index(row, sid_col));
    return selection;
  }
}

void NameMap::refreshModelRenaming(
        const std::unordered_map<int, std::string>& sid2nogood,
        const QTableView& table,
        int sid_col, bool expand_expressions,
        const std::function <void (int, QString)>& tsi) const {
  const QModelIndexList selection = TableHelpers::getSelection(table, sid_col);
  for(int i=0; i<selection.count(); i++) {
    int row = selection.at(i).row();
    int64_t sid = table.model()->data(table.model()->index(row, sid_col)).toLongLong();

    auto ng_item = sid2nogood.find(sid);
    if (ng_item == sid2nogood.end()) {
      continue;  /// nogood not found
    }

    QString qclause = QString::fromStdString(ng_item->second);
    QString clause = replaceNames(qclause, expand_expressions);

    tsi(row, clause);
  }
  TableHelpers::updateSelection(table);
}

const QString NameMap::getHeatMapFromModel(
        std::unordered_map<int64_t, std::string*>& sid2info,
        const QTableView& table,
        int sid_col) const {

  const QModelIndexList selection = TableHelpers::getSelection(table, sid_col);
  QStringList label;
  std::unordered_map<int, int> con_ids;

  int max_count = 0;
  for(int i=0; i<selection.count(); i++) {
    int row = selection.at(i).row();
    int64_t sid = table.model()->data(table.model()->index(row, sid_col)).toLongLong();
    auto info_item = sid2info.find(sid);
    if(info_item != sid2info.end()) {
      std::string info_text = *info_item->second;
      auto info_json = nlohmann::json::parse(info_text);

      auto reasonIt = info_json.find("reasons");
      if(reasonIt != info_json.end()) {
        auto reasons = *reasonIt;
        for(int con_id : reasons) {
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
      }

      label << QString::number(sid);
    }
  }

  TableHelpers::updateSelection(table);
  return getHeatMap(con_ids, max_count, label.join(" "));
}



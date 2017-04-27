#include "namemap.hh"

#include <qstringlist.h>
#include <qtextedit.h>
#include <math.h>
#include <qtextstream.h>
#include <qvector.h>
#include <qlineedit.h>

#include "third-party/json.hpp"

#define reg_mzn_ident "[A-Za-z][A-Za-z0-9_]*"
#define reg_number "[0-9]*"

QRegExp NameMap::var_name_regex(reg_mzn_ident);
QRegExp NameMap::assignment_regex(reg_mzn_ident "=" reg_number);

QList<int> getReasons(const int64_t sid,
                      const std::unordered_map<int64_t, std::string*>& sid2info) {
  QList<int> reason_list;
  auto info_item = sid2info.find(sid);
  if(info_item != sid2info.end()) {
    auto info_json = nlohmann::json::parse(*info_item->second);
    auto reasonIt = info_json.find("reasons");
    if(reasonIt != info_json.end()) {
      auto reasons = *reasonIt;
      for(int con_id : reasons) {
        reason_list.append(con_id);
      }
    }
  }
  return reason_list;
}

static const Location empty_location;
bool Location::contains(const Location& loc) const {
  return ((sl  < loc.sl) ||
          (sl == loc.sl && sc <= loc.sc)) &&
         ((el  > loc.el) ||
          (el == loc.el && ec >= loc.ec));
}

Location::Location() {}
Location::Location(const QString& pathHead) {
  const QStringList splitHead = pathHead.split(":");
  path = splitHead[0];
  sl = splitHead[1].toInt();
  sc = splitHead[2].toInt();
  el = splitHead[3].toInt();
  ec = splitHead[4].toInt();
}
Location Location::fromString(const QString& text) {
  Location loc;
  const QStringList leftRight = text.split("-");
  {
    const QString& start = leftRight[0];
    QStringList slc = start.split(":");
    loc.sl = slc[0].toInt();
    loc.sc = slc.size() > 1 ? slc[1].toInt() : 0;
  }

  if(leftRight.size() > 1) {
    const QString& end = leftRight[1];
    QStringList elc = end.split(":");
    loc.el = elc[0].toInt();
    loc.ec = elc.size() > 1 ? elc[1].toInt() : std::numeric_limits<int>::max();
  } else {
    loc.el = loc.sl;
    loc.ec = std::numeric_limits<int>::max();
  }

  return loc;
}

QString Location::toString() const {
  return QString("%1:%2-%3:%4").arg(sl).arg(sc).arg(el).arg(ec);
}

LocationFilter::LocationFilter() {}
LocationFilter::LocationFilter(const QList<Location> locations) {
  for(const Location& newLoc : locations) {
    if(_locations.empty() || !contains(newLoc)) {
      _locations.append(locations);
    }
  }
}

bool LocationFilter::contains(const Location& loc) const {
  if(_locations.empty()) return true;
  for(const Location& floc : _locations)
    if(floc.contains(loc)) return true;
  return false;
}

LocationFilter LocationFilter::fromString(const QString& text) {
  QList<Location> locs;
  for(const QString& locString : text.split(",", QString::SkipEmptyParts)) {
    locs.append(Location::fromString(locString));
  }
  return LocationFilter(locs);
}

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
      Location loc(getPathHead(s[2], false).last());
      _nameMap[s[0]] = std::make_tuple(s[1], s[2], loc);
      if(s[1].left(12) == "X_INTRODUCED") {
        addIdExpressionToMap(s[0], modelText);
      }
    }
  }
}

static const QString empty_string;
const QString& NameMap::getNiceName(const QString& ident) const {
  auto it = _nameMap.find(ident);
  if(it != _nameMap.end()) {
    return std::get<0>(it.value());
  }
  return empty_string;
}

const QString& NameMap::getPath(const QString& ident) const {
  auto it = _nameMap.find(ident);
  if(it != _nameMap.end()) {
    return std::get<1>(it.value());
  }
  return empty_string;
}

const Location& NameMap::getLocation(const QString& ident) const {
  auto it = _nameMap.find(ident);
  if(it != _nameMap.end()) {
    return std::get<2>(it.value());
  }
  return empty_location;
}

const QString NameMap::replaceNames(const QString& text, bool expand_expressions) const {
  if (_nameMap.size() == 0) {
    return text;
  }

  QStringList ss;
  int pos = 0;
  int prev = 0;
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

  int pos = 0;
  while ((pos = assignment_regex.indexIn(path, pos)) != -1) {
    const QString& assign =  assignment_regex.cap(0);
    const QStringList leftright = assign.split("=");
    st[leftright[0]] = std::make_tuple(leftright[1], "", empty_location);
    pos += assignment_regex.matchedLength();
  }

  const NameMap nm(st);
  return nm.replaceNames(expression);
}

void NameMap::addIdExpressionToMap(const Ident& ident, const std::vector<QString>& modelText) {
  if(modelText.size() == 0) return;

  const Location& loc = getLocation(ident);
  QString expression = modelText[static_cast<size_t>(loc.sl-1)].mid(loc.sc-1, loc.ec-(loc.sc-1));
  const QStringList components = getPathHead(getPath(ident), true);
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

#include "namemap.hh"

#include <qstringlist.h>
#include <qtextedit.h>
#include <math.h>
#include <qtextstream.h>
#include <qvector.h>

QRegExp NameMap::var_name_regex("[A-Za-z][A-Za-z0-9_]*");

NameMap::NameMap(QString& path_filename, QString& model_filename) : _model_filename(model_filename) {
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
      if(s[0] == s[1] && s[1].left(12) == "X_INTRODUCED") {
        addIdExpressionToMap(s[0]);
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
    if(name == id && modelText.size() > 0 && expand_expressions) {
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

void NameMap::addIdExpressionToMap(const Ident& ident) {
  if(modelText.size() == 0) return;
  const QString& path = _nameMap[ident].second;
  const QString loc = getPathHead(path, false);

  QStringList locList = loc.split(":");
  bool ok;
  int sl = locList[1].toInt(&ok);
  int sc = locList[2].toInt(&ok);
  int el = locList[3].toInt(&ok);
  int ec = locList[4].toInt(&ok);

  // Extract text and store it in map
  QString newName = modelText[sl-1].mid(sc-1, ec-(sc-1));
  _expressionMap.insert(ident, newName);
}

// Get the most specific path component that is still in the user model
const QString NameMap::getPathHead(const QString& path, bool includeTrail = false) const {
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
      return previousHead.join("");

    if(!includeTrail)
      previousHead.clear();
    previousHead += path_head;
    i++;
  } while(i < pathSplit.size());

  return previousHead.join("");
}

const QString NameMap::getHeatMap(
    std::unordered_map<int, int> con_id_counts,
    int max_count,
    const QString& desc) const {
  int bucket = int(ceil(255.0/double(max_count+1)));
  QStringList highlight_url;
  highlight_url << "<a href=\"highlight://?";
  for(auto it : con_id_counts) {
    const QString con_string = QString::number(it.first);
    const QString path = getPath(con_string);
    const QString path_head = getPathHead(path);
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

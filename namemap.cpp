#include "namemap.hh"

#include <qstringlist.h>
#include <qtextedit.h>
#include <math.h>
#include <qtextstream.h>
#include <qvector.h>

std::regex NameMap::var_name_regex("[A-Za-z][A-Za-z0-9_]*");

const std::string NameMap::getNiceName(std::string s) const {
  auto it = _nameMap.find(s);
  if(it != _nameMap.end()) {
      return it->second.first;
  }
  return "";
}

const std::string NameMap::getPath(std::string s) const {
    auto it = _nameMap.find(s);
    if(it != _nameMap.end()) {
        return it->second.second;
    }
    return "";
}


const std::string NameMap::replaceNames(std::string text) const {
    if (_nameMap.size() == 0) {
      return text;
    }

    std::regex_iterator<std::string::const_iterator> rit(text.begin(), text.end(), var_name_regex);
    std::regex_iterator<std::string::const_iterator> rend;

    std::stringstream ss;
    long prev = 0;
    while(rit != rend) {
      long pos = rit->position();
      ss << text.substr(prev, pos-prev);
      std::string id = rit->str();
      std::string name = getNiceName(id);
      //if(name == id && modelText.size() > 0) {
      //    name = path2Expression(QString().fromStdString(getPath(id))).toStdString();
      //    if(!name.empty()) {
      //        name = "〈" + name + "〉";
      //    } else {
      //        name = id;
      //    }
      //}
      ss << name;
      prev = pos + id.length();
      ++rit;
    }
    ss << text.substr(prev, text.size());
    return ss.str();
}

const QString NameMap::path2Expression(const QString& path) const {
    QString newName;
    QStringList components = path.split(";");

    const QString loc = getPathHead(path, false);
    //int idx = components.size() - 1;
    //do { loc = components[--idx]; } while(loc.size() == 0);

    QStringList locList = loc.split(":");
    QString filename = locList[0];
    bool ok;
    int sl = locList[1].toInt(&ok);
    int sc = locList[2].toInt(&ok);
    int el = locList[3].toInt(&ok);
    int ec = locList[4].toInt(&ok);

    if(!ok) return "";

    // Extract text from file
    newName = modelText[sl].mid(sc-1, ec);

    return newName;
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
    std::string con_string = std::to_string(it.first);
    QString path = QString::fromStdString(getPath(con_string));
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

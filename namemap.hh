#ifndef NAMEMAP_HH
#define NAMEMAP_HH

#include <unordered_map>
#include <utility>
#include <regex>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>

#include <iostream>

using std::string;
using std::pair;
using std::unordered_map;
using std::regex;


class NameMap {
public:
    using Map = unordered_map<string, pair<string, string> >;

    NameMap() {}
    NameMap(QString& path_filename, QString& model_filename) : filename(model_filename) {
      if(path_filename != "") {
        QFile pf(path_filename);
        if(pf.open(QIODevice::ReadOnly)) {
          QTextStream in(&pf);
          while(!in.atEnd()) {
            QString line = in.readLine();
            QStringList s = line.split("\t");
            _nameMap[s[0].toStdString()] = std::make_pair(s[1].toStdString(), s[2].toStdString());
          }
        }
      }

      QFile model_file(model_filename);
      if(model_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream tstream(&model_file);
        QString line = tstream.readLine();
        while (!line.isNull()) {
          modelText.push_back(line);
          line = tstream.readLine();
        }
      }
    }

    const string getPath(string s) const;
    const string getNiceName(string s) const;
    const string replaceNames(string text) const;
    const QString getHeatMap(
            std::unordered_map<int, int> con_id_counts,
            int max_count,
            const QString& desc) const;

    const QString getPathHead(const QString& path, bool includeTrail) const;
    const QString path2Expression(const QString& path) const;

private:
    static regex var_name_regex;
    QString filename;
    Map _nameMap;
    std::vector<QString> modelText;
};

#endif // NAMEMAP_HH

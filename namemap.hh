#ifndef NAMEMAP_HH
#define NAMEMAP_HH

#include <unordered_map>
#include <utility>
#include <regex>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>

#include <iostream>
#include <qsortfilterproxymodel.h>
#include <qstandarditemmodel.h>
#include <qtableview.h>

#include <set>

struct Location {
  //QString path = "";
  int sl = 0;
  int sc = 0;
  int el = 0;
  int ec = 0;

  Location();
  Location(const Location& l);
  Location(const QString& pathHead);
  bool contains(const Location& loc) const;
  bool containsStart(const Location& loc) const;
  void mergeStart(const Location& loc);
  void mergeEnd(const Location& loc);

  static Location fromString(const QString& text);
  QString toString() const;

  bool operator==(const Location& loc) const;
};

inline uint qHash(const Location& l) {
  return qHash(l.sl) ^ qHash(l.sc) ^ qHash(l.el) ^ qHash(l.ec);
}

class LocationFilter {
public:
  LocationFilter();
  LocationFilter(QList<Location> locations);
  bool contains(const Location& loc) const;

  static LocationFilter fromString(const QString& text);
  QString toString() const;

private:
  QSet<Location> _loc_filters;
};

// This should go somewhere more sensible
QList<int> getReasons(const int64_t sid,
                      const std::unordered_map<int64_t, std::string*>& sid2info);

class NameMap {
public:
  using Path = QString;
  using NiceName = QString;
  using Ident = QString;
  using Expression = QString;
  using SymbolTable = QHash<Ident, std::tuple<NiceName, Path, Location> >;
  using ExpressionTable = QHash<Ident, Expression>;

  NameMap() {}
  NameMap(QString& path_filename, QString& model_filename);
  NameMap(SymbolTable& st);

  const QString& getPath(const Ident& ident) const;
  const Location& getLocation(const Ident& ident) const;
  const Location& getLocation(const int cid) const;
  const QString& getNiceName(const Ident& ident) const;
  QString replaceNames(const QString& text, bool expand_expressions = false) const;
  QString getHeatMap(const std::unordered_map<int, int>& con_id_counts,
                     int max_count, const QString& desc) const;
  QList<Location> getLocations(const QList<int>& reasons) const;
  QString getLocationFilterString(const QList<int>& reasons) const;

private:
  QStringList getPathHead(const Path& path, bool includeTrail) const;
  QString replaceAssignments(const QString& path, const QString& expression) const;
  void addIdExpressionToMap(const Ident& ident, const std::vector<QString>& modelText);

  static QRegExp var_name_regex;
  static QRegExp assignment_regex;
  SymbolTable _nameMap;
  ExpressionTable _expressionMap;
};

#endif // NAMEMAP_HH
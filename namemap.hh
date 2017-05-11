#ifndef NAMEMAP_HH
#define NAMEMAP_HH

#include <qstring.h>
#include <qvector.h>
#include <qset.h>
#include <unordered_map>
#include <vector>

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

uint qHash(const Location& l);

class LocationFilter {
public:
  LocationFilter();
  LocationFilter(const QSet<Location>& locations);
  bool contains(const Location& loc) const;

  static LocationFilter fromString(const QString& text);
  QString toString() const;

private:
  QSet<Location> _loc_filters;
};

// This should go somewhere more sensible
QVector<int> getReasons(const int64_t sid,
                      const std::unordered_map<int64_t, std::string*>& sid2info);

class NameMap {
private:
struct SymbolRecord {
  QString niceName;
  QString path;
  Location location;

  SymbolRecord();
  SymbolRecord(const QString&, const QString&, const Location&);
};

public:
  using SymbolTable = QHash<QString, SymbolRecord >;
  using ExpressionTable = QHash<QString, QString>;

  NameMap() {}
  NameMap(QString& path_filename, QString& model_filename);
  NameMap(SymbolTable& st);

  const QString& getPath(const QString& ident) const;
  const Location& getLocation(const QString& ident) const;
  const Location& getLocation(const int cid) const;
  const QString& getNiceName(const QString& ident) const;
  QString replaceNames(const QString& text, bool expand_expressions = false) const;
  QString getHeatMap(const std::unordered_map<int, int>& con_id_counts, int max_count) const;
  QSet<Location> getLocations(const QVector<int>& reasons) const;
  QString getLocationFilterString(const QVector<int>& reasons) const;

private:
  QStringList getPathHead(const QString& path, bool includeTrail) const;
  QString replaceAssignments(const QString& path, const QString& expression) const;

  void addIdExpressionToMap(const QString& ident, const std::vector<QString>& modelText);

  static QRegExp var_name_regex;
  static QRegExp assignment_regex;
  SymbolTable _nameMap;
  ExpressionTable _expressionMap;
};

#endif // NAMEMAP_HH

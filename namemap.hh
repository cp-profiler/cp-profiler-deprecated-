#ifndef NAMEMAP_HH
#define NAMEMAP_HH

#include <unordered_map>
#include <utility>
#include <regex>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>

#include <iostream>

using std::pair;
using std::unordered_map;
using std::regex;

class NameMap {
public:
  using Path = QString;
  using NiceName = QString;
  using Ident = QString;
  using Expression = QString;
  using SymbolTable = QHash<Ident, pair<NiceName, Path> >;
  using ExpressionTable = QHash<Ident, Expression>;

  NameMap() {}
  NameMap(QString& path_filename, QString& model_filename);

  const QString getPath(const Ident& ident) const;
  const QString getNiceName(const Ident& ident) const;
  const QString replaceNames(const QString& text) const;
  const QString getHeatMap(
      std::unordered_map<int, int> con_id_counts,
      int max_count,
      const QString& desc) const;

private:
  const QString getPathHead(const Path& path, bool includeTrail) const;
  void addIdExpressionToMap(const Ident& ident);

  static QRegExp var_name_regex;
  QString _model_filename;
  SymbolTable _nameMap;
  ExpressionTable _expressionMap;
  std::vector<QString> modelText;
};

#endif // NAMEMAP_HH

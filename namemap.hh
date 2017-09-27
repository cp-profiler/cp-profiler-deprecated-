#ifndef NAMEMAP_HH
#define NAMEMAP_HH

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <regex>

#include "cpprofiler/universal.hh"

struct Location {
  //QString path = "";
  int sl = 0;
  int sc = 0;
  int el = 0;
  int ec = 0;

  Location();
  Location(const Location& l);
  Location(const std::string& pathHead);
  bool contains(const Location& loc) const;
  bool containsStart(const Location& loc) const;
  void mergeStart(const Location& loc);
  void mergeEnd(const Location& loc);

  static Location fromString(const std::string& text);
  std::string toString() const;

  bool operator==(const Location& loc) const;
  bool operator<(const Location& loc) const;
};
namespace std {
  template<> struct hash<Location> {
    size_t operator()(Location const& l) const;
  };
}

class LocationFilter {
public:
  LocationFilter();
  LocationFilter(const std::unordered_set<Location>& locations);
  bool contains(const Location& loc) const;

  static LocationFilter fromString(const std::string& text);
  std::string toString() const;

private:
  std::unordered_set<Location> _loc_filters;
};

// This should go somewhere more sensible
std::vector<int> getReasons(const std::string* maybe_info);

class NameMap {
private:
struct SymbolRecord {
  std::string niceName;
  std::string path;
  Location location;

  SymbolRecord();
  SymbolRecord(const std::string&, const std::string&, const Location&);
};

public:
  using SymbolTable = std::unordered_map<std::string, SymbolRecord>;
  using ExpressionTable = std::unordered_map<std::string, std::string>;

  NameMap() {}
  NameMap(const std::string& path_filename, const std::string& model_filename);

  const std::string& getPath(const std::string& ident) const;
  const Location& getLocation(const std::string& ident) const;
  const Location& getLocation(const int cid) const;
  const std::string& getNiceName(const std::string& ident) const;
  std::string replaceNames(const std::string& text, bool expand_expressions = false) const;
  std::string getHeatMap(const std::unordered_map<int, int>& con_id_counts, int max_count) const;
  std::unordered_set<Location> getLocations(const std::vector<int>& reasons) const;
  std::string getLocationFilterString(const std::vector<int>& reasons) const;

private:
  NameMap(const SymbolTable& st);

  std::vector<std::string> getPathHead(const std::string& path, bool includeTrail) const;
  std::string replaceAssignments(const std::string& path, const std::string& expression) const;

  void addIdExpressionToMap(const std::string& ident, const std::vector<std::string>& modelText);

  static std::regex var_name_regex;
  static std::regex assignment_regex;
  SymbolTable _nameMap;
  ExpressionTable _expressionMap;
};

#endif // NAMEMAP_HH

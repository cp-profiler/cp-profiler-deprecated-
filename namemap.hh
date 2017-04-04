#ifndef NAMEMAP_HH
#define NAMEMAP_HH

#include <unordered_map>
#include <utility>
#include <regex>

using std::string;
using std::pair;
using std::unordered_map;
using std::regex;

class NameMap {
public:
    using Map = unordered_map<string, pair<string, string> >;

    NameMap() {}
    NameMap(Map& nm) : _nameMap(nm) {}

    const string getPath(string s) const;
    const string getNiceName(string s) const;
    const string replaceNames(string text) const;
    const string getHeatMap(string nogoodString) const;

private:
    static regex var_name_regex;
    Map _nameMap;
};

#endif // NAMEMAP_HH

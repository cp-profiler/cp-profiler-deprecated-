#include <fstream>
#include <sstream>
#include <string>

#include "namemap.hh"
#include "third-party/json.hpp"

#include "cpprofiler/utils/string_utils.hh"

#define reg_mzn_ident "[A-Za-z][A-Za-z0-9_]*"
#define reg_number "[0-9]*"

#define minor_sep '|'
#define major_sep ';'

std::regex NameMap::var_name_regex(reg_mzn_ident);
std::regex NameMap::assignment_regex(reg_mzn_ident "=" reg_number);

using std::string;
using std::vector;
using std::unordered_set;

vector<int> getReasons(const string* maybe_info) {

  if (maybe_info == nullptr) return {};

  vector<int> reason_list;

  auto info_json = nlohmann::json::parse(*maybe_info);
  auto reasonIt = info_json.find("reasons");
  if(reasonIt != info_json.end()) {
    auto reasons = *reasonIt;
    for(int con_id : reasons) {
      reason_list.push_back(con_id);
    }
  }
  return reason_list;
}

static const Location empty_location;
bool Location::contains(const Location& loc) const {
  return ((sl  < loc.sl) || (sl == loc.sl && sc <= loc.sc)) &&
         ((el  > loc.el) || (el == loc.el && ec >= loc.ec));
}

bool Location::containsStart(const Location& loc) const {
  return ((sl  < loc.sl) || (sl == loc.sl && sc <= loc.sc)) &&
         ((el  > loc.sl) || (el == loc.sl && ec >= loc.sc));
}

void Location::mergeEnd(const Location& loc) {
  el = loc.el;
  ec = loc.ec;
}
void Location::mergeStart(const Location& loc) {
  sl = loc.sl;
  sc = loc.sc;
}

bool Location::operator==(const Location& loc) const {
  return sl == loc.sl && sc == loc.sc && el == loc.el && ec == loc.ec;
}
bool Location::operator<(const Location& loc) const {
  if(sl < loc.sl) return true;
  if(sl == loc.sl) {
    if(sc < loc.sc) return true;
    if(sc == loc.sc) {
      if(el < loc.el) return true;
      if(el == loc.el) {
        return ec <= loc.ec;
      }
    }
  }
  return false;
}

Location::Location() {}
Location::Location(const Location& l) : sl(l.sl), sc(l.sc), el(l.el), ec(l.ec) {}
Location::Location(const string& pathHead) {
  const vector<string> splitHead = utils::split(pathHead, minor_sep);
  //path = splitHead[0];
  sl = stoi(splitHead[1]);
  sc = stoi(splitHead[2]);
  el = stoi(splitHead[3]);
  ec = stoi(splitHead[4]);
}
Location Location::fromString(const string& text) {
  Location loc;
  const vector<string> leftRight = utils::split(text, '-');
  const string& start = leftRight[0];
  vector<string> slc = utils::split(start, minor_sep);
  loc.sl = stoi(slc[0]);
  loc.sc = slc.size() > 1 ? stoi(slc[1]) : 0;

  if(leftRight.size() > 1) {
    const string& end = leftRight[1];
    vector<string> elc = utils::split(end, minor_sep);
    loc.el = stoi(elc[0]);
    loc.ec = elc.size() > 1 ? stoi(elc[1]) : std::numeric_limits<int>::max();
  } else {
    loc.el = loc.sl;
    loc.ec = slc.size() > 1 ? loc.sc : std::numeric_limits<int>::max();
  }

  return loc;
}

string Location::toString() const {
  std::stringstream locSL;
  locSL << sl;
  if(sc > 0) locSL << minor_sep << sc;
  if(sl != el) {
    locSL << "-" << el;
    if(ec != std::numeric_limits<int>::max()) locSL << minor_sep << ec;
  } else if(sc != ec && ec != std::numeric_limits<int>::max()) {
    locSL << "-" << el;
    if(ec != std::numeric_limits<int>::max()) locSL << minor_sep << ec;
  }
  return locSL.str();
}

string LocationFilter::toString() const {
  vector<string> locStrings;
  for(const Location& loc : _loc_filters)
    locStrings.push_back(loc.toString());
  return utils::join(locStrings, ',');
}

size_t std::hash<Location>::operator()(Location const& l) const {
  return 32*(32*(32*(32
                     +std::hash<int>()(l.sl))
                 +std::hash<int>()(l.sc))
             +std::hash<int>()(l.el))
          +std::hash<int>()(l.ec);
}

LocationFilter::LocationFilter() {}
LocationFilter::LocationFilter(const unordered_set<Location>& locations) {
  for(const Location& cl1 : locations) {
    Location l1 = cl1;
    if(_loc_filters.find(l1) == _loc_filters.end()) {
      unordered_set<Location> temp_filters (_loc_filters);
      for(const Location& l2 : temp_filters) {
        if(l1.contains(l2)) {
          _loc_filters.erase(l2);
        } else if(l2.contains(l1)) {
          l1 = l2;
          break;
        } else if(l1.containsStart(l2)) {
          _loc_filters.erase(l2);
          l1.mergeEnd(l2);
        } else if(l2.containsStart(l1)) {
          _loc_filters.erase(l2);
          l1.mergeStart(l2);
        }
      }
      _loc_filters.insert(l1);
    }
  }
}

bool LocationFilter::contains(const Location& loc) const {
  if(_loc_filters.empty()) return true;
  for(const Location& l : _loc_filters) {
    if(l.contains(loc)) return true;
  }
  return false;
}

LocationFilter LocationFilter::fromString(const string& text) {
  unordered_set<Location> locs;
  for(const string& locString : utils::split(text, ','))
    locs.insert(Location::fromString(locString));
  return LocationFilter(locs);
}

NameMap::SymbolRecord::SymbolRecord()
    : niceName(""), path(""), location() {};
NameMap::SymbolRecord::SymbolRecord(const std::string& nn, const std::string& p, const Location& l)
    : niceName(nn), path(p), location(l) {};

NameMap::NameMap(const SymbolTable& st) : _nameMap(st) {}

NameMap::NameMap(const std::string& path_filename, const std::string& model_filename) {
  vector<std::string> modelText;
  std::ifstream model_file(model_filename);
  if(model_file.is_open()) {
    string line;
    while (getline(model_file, line))
      modelText.push_back(line);
    model_file.close();
  }

  std::ifstream pf(path_filename);
  if(pf.is_open()) {
    string line;
    while(getline(pf, line)) {
      vector<string> s = utils::split(line, '\t');
      Location loc(getPathHead(s[2], false).back());
      _nameMap[s[0]] = SymbolRecord(s[1], s[2], loc);
      if(s[1].substr(0, 12) == "X_INTRODUCED")
        addIdExpressionToMap(s[0], modelText);
    }
  }
}

static const string empty_string;
const string& NameMap::getNiceName(const string& ident) const {
  auto it = _nameMap.find(ident);
  if(it != _nameMap.end()) {
    return it->second.niceName;
  }
  return empty_string;
}

bool NameMap::isEmpty() const {
  return _nameMap.empty();
}

const string& NameMap::getPath(const string& ident) const {
  auto it = _nameMap.find(ident);
  if(it != _nameMap.end()) {
    return it->second.path;
  }
  return empty_string;
}

const Location& NameMap::getLocation(const int cid) const {
  return getLocation(std::to_string(cid));
}

const Location& NameMap::getLocation(const string& ident) const {
  auto it = _nameMap.find(ident);
  if(it != _nameMap.end()) {
    return it->second.location;
  }
  return empty_location;
}

string NameMap::replaceNames(const string& text, bool expand_expressions) const {
  if (_nameMap.size() == 0) return text;

  std::stringstream ss;
  size_t pos = 0;

  auto var_names_begin = std::sregex_iterator(text.begin(), text.end(), var_name_regex);
  auto var_names_end = std::sregex_iterator();

  for(std::sregex_iterator i = var_names_begin; i != var_names_end; i++) {
    std::smatch match = *i;
    ss << text.substr(pos, static_cast<size_t>(match.position())-pos);
    const string& id = match.str();
    string name = getNiceName(id);

    if(expand_expressions && name.substr(0, 12) == "X_INTRODUCED") {
      auto eit = _expressionMap.find(name);
      if(eit != _expressionMap.end()) {
        std::stringstream ss;
        ss << "\'" << eit->second << "\'";
        name = ss.str();
      } else {
        name = id;
      }
    }
    ss << (name != "" ? name : id);
    pos = static_cast<size_t>(match.position() + match.length());
  }

  ss << text.substr(pos, text.size());
  return ss.str();
}

string NameMap::replaceAssignments(const string& path, const string& expression) const {
  NameMap::SymbolTable st;

  auto assignment_begin = std::sregex_iterator(path.begin(), path.end(), assignment_regex);
  auto assignment_end = std::sregex_iterator();

  for(std::sregex_iterator i = assignment_begin; i != assignment_end; i++) {
      std::smatch match = *i;
      const string& assign = match.str();
      const vector<string> leftright = utils::split(assign, '=');
      st[leftright[0]] = SymbolRecord(leftright[1], "", empty_location);
  }

  const NameMap nm(st);
  return nm.replaceNames(expression);
}

void NameMap::addIdExpressionToMap(const string& ident, const vector<string>& modelText) {
  if(modelText.size() == 0) return;

  const Location& loc = getLocation(ident);
  string expression = modelText[static_cast<size_t>(loc.sl-1)].substr(
              static_cast<size_t>(loc.sc-1),
              static_cast<size_t>(loc.ec-(loc.sc-1)));
  const vector<string> components = getPathHead(getPath(ident), true);
  expression = replaceAssignments(utils::join(components, major_sep), expression);

  _expressionMap.insert(make_pair(ident, expression));
}

// Get the most specific path component that is still in the user model
vector<string> NameMap::getPathHead(const string& path, bool includeTrail = false) const {
  vector<string> pathSplit = utils::split(path, major_sep);

  string mzn_file;
  vector<string> previousHead;

  if(pathSplit.size() == 0)
    return previousHead;

  size_t i=0;
  do {
    string path_head = pathSplit[i];
    vector<string> head = utils::split(path_head, minor_sep);
    string head_file;
    if(head.size() > 0) {
      if(i==0) mzn_file = head[0];
      head_file = head[0];
    }

    if(head_file != mzn_file)
      return previousHead;

    if(!includeTrail)
      previousHead.clear();
    previousHead.push_back(path_head);
    i++;
  } while(i < pathSplit.size());

  return previousHead;
}

string NameMap::getHeatMap(
    const std::unordered_map<int, int>& con_id_counts, int max_count) const {
  int bucket = int(ceil(255.0/double(max_count+1)));

  std::unordered_map<string, int> locations;
  for(auto it : con_id_counts) {
    const string path = getPath(std::to_string(it.first));
    const vector<string> path_head_elements = getPathHead(path, false);
    if(path_head_elements.size() == 0)
      continue;
    const string path_head = path_head_elements[0];
    vector<string> location_etc = utils::split(path_head, minor_sep);
    int count = it.second;

    if(location_etc.size() >= 5) {
      vector<string> newLoc;
      for(int i=0; i<5; i++) newLoc.push_back(location_etc[static_cast<size_t>(i)]);
      int val = (1 + count) * bucket;
      val = val <= 255 ? val : 255;
      string loc_str = utils::join(newLoc, minor_sep);
      std::unordered_map<string, int>::iterator loc_it = locations.find(loc_str);
      if(loc_it == locations.end()) {
        locations[loc_str] = val;
      } else {
        loc_it->second = loc_it->second > val ? loc_it->second : val;
      }
    }
  }

  std::stringstream highlight_url;
  highlight_url << "highlight://?";
  for(auto it : locations)
    highlight_url << it.first << minor_sep << it.second << ";";

  return highlight_url.str();
}

unordered_set<Location> NameMap::getLocations(const vector<int>& reasons) const {
  unordered_set<Location> locations;
  for(int cid : reasons)
    locations.insert(getLocation(cid));
  return locations;
}

string NameMap::getLocationFilterString(const vector<int>& reasons) const {
  return LocationFilter(getLocations(reasons)).toString();
}

#include "namemap.hh"

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
      ss << (name != "" ? name : id);
      prev = pos + id.length();
      ++rit;
    }
    ss << text.substr(prev, text.size());
    return ss.str();
}

// This should be moved somewhere better
const std::string NameMap::getHeatMap(std::string nogoodString) const {
  std::stringstream url;
  url << "<a href=\"";
  url << "\">" << getNiceName("1") << "</a>";

  std::string urlString = url.str();
  return urlString;
}

#include <sstream>
#include "cpprofiler/utils/string_utils.hh"

using std::vector;
using std::string;

namespace utils {
vector<string> split(const string& str, char delim) {

  std::stringstream ss;
  ss.str(str);
  std::string item;

  vector<string> result;

  auto inserter = std::back_inserter(result);

  while (std::getline(ss, item, delim)) {
    *(inserter++) = item;
  }

  return result;
}

string join(const vector<string>& strs, char sep) {
  std::stringstream ss;
  for(size_t i=0; i<strs.size(); i++) {
    if(i) ss << sep;
    ss << strs[i];
  }
  return ss.str();
}

}

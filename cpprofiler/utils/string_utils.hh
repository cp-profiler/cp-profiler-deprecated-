#ifndef STRING_UTILS_HH
#define STRING_UTILS_HH

#include <vector>
#include <string>

namespace utils {
  std::vector<std::string> split(const std::string& str, char delim, bool include_empty = false);
  std::string join(const std::vector<std::string>& strs, char sep);
}

#endif // STRING_UTILS_HH

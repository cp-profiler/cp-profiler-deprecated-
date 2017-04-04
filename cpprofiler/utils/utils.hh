#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace utils {

  std::vector<std::string> split(const std::string &s, char delim);

}

template<typename K, typename T>
void print(std::ostream& os, const std::unordered_map<K, T>& map) {
  for (auto& k : map) {
    os << k.first << "\t" << k.second << "\n";
  }
}

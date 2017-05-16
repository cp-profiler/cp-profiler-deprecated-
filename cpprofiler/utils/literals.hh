#pragma once

#include <string>
#include <vector>

namespace utils { namespace lits {

  void test_module();

  struct Lit {
    std::string var;
    std::string op;
    int val;
    bool is_bool;
  };

  Lit parse_lit(const std::string& lit);

  std::string simplify_ng(const std::string& ng);
  std::vector<Lit> apply_rules_same_var(const std::string& var, const std::vector<Lit>& lits);
  bool operator==(const Lit& lhs, const Lit& rhs);
  bool operator<(const Lit& lhs, const Lit& rhs);

  std::vector<std::string> split(const std::string& str, char delim);
  std::string join(const std::vector<std::string>& strs, char sep);
}}

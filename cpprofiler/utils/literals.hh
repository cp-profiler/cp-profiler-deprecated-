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
    bool is_clean;
  };

  Lit parse_lit(const std::string& lit);

  std::string remove_redundant_wspaces(const std::string& ng);
  std::string simplify_ng(const std::string& ng);
  std::vector<Lit> apply_rules_same_var(const std::string& var, const std::vector<Lit>& lits);
  bool operator==(const Lit& lhs, const Lit& rhs);
  bool operator<(const Lit& lhs, const Lit& rhs);
  std::string stringify_lits(const std::vector<Lit>& lits);
  Lit negate_lit(const Lit& l);
}}

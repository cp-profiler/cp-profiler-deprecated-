#pragma once

#include <string>

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


}}
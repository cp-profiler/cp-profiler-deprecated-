#include "literals.hh"
#include <QDebug>
#include <string>
#include <set>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <functional>
#include <iostream>
#include <climits>
#include <stdexcept>
#include <array>

#include "libs/perf_helper.hh"
#include "cpprofiler/utils/string_utils.hh"

using std::string; using std::vector;
using std::pair;

using std::cout; using std::endl; using std::ostream;

namespace utils { namespace lits {

  static void test_parse_lit();
  static void test_simplify_ng();

  ostream& operator<<(ostream& os, const Lit& l) {
    return os << l.var << l.op << l.val << " is_bool:" << l.is_bool;
  }

  bool operator<(const Lit& lhs, const Lit& rhs) {
    if(lhs.var < rhs.var) return true;
    if(lhs.var == rhs.var) {
      if(lhs.op < rhs.op) return true;
      if(lhs.op == rhs.op) {
        if(lhs.val < rhs.val) return true;
        if(lhs.val == rhs.val) {
          return lhs.is_bool < rhs.is_bool;
        }
      }
    }
    return false;
  }

  template<typename T>
  ostream& operator<<(ostream& os, const vector<T>& v) {
    for (auto& e : v) {
      os << e << ",";
    }

    return os;
  }

  constexpr int N_LITS = 7;

  static const std::array<string, N_LITS> ops    {{">=", "<=", "!=", "==",  ">",  "<",  "="}};

  static const std::array<string, N_LITS> neg_ops{{"<",  ">",  "=", "!=", "<=", ">=", "!="}};

  static string negate_op(const string& op) {
    for (auto i = 0u; i < N_LITS; ++i) {
      if (op == ops[i]) {
        return neg_ops[i];
      }
    }

    return "?";
  }

  bool operator==(const Lit& lhs, const Lit& rhs) {
    return (lhs.var == rhs.var) && (lhs.op == rhs.op) && (lhs.val == rhs.val);
  }

  Lit parse_lit(const string& lit) {
    size_t end_lhs = 0;

    if(lit[0] == '\'') {
      end_lhs = lit.find_last_of('\'');
    }

    for (auto& op : ops) {
      auto pos = lit.find(op, end_lhs);
      if (pos != string::npos) {

        auto var = lit.substr(0, pos);
        auto str_val = lit.substr(pos + op.size());

        int val;
        bool is_bool = false;
        if (str_val == "false") {
          val = 0; is_bool = true;
        } else if (str_val == "true") {
          val = 1; is_bool = true;
        } else {
          try {
            val = std::stoi(str_val);
          } catch (std::invalid_argument&) {
            return {lit, "", 0, false, false};
          }
        }

        return {std::move(var), op, std::move(val), is_bool, true};
      }
    }
    return {lit, "", 0, false, false};
  }

  static void trim(string& str) {
    auto pos_l = str.find_first_not_of(" \t");
    auto pos_r = str.find_last_not_of(" \t");

    str = str.substr(pos_l, pos_r - pos_l + 1);
  }

  template<typename T, typename R, typename Fun>
  vector<R> map(const vector<T>& v, const Fun& f) {

    vector<R> result;
    result.reserve(v.size());

    for (auto& e : v) {
      result.push_back(f(e));
    }

    return result;
  }

  template<typename T>
  vector<T> uniq(const vector<T>& v) {

    std::set<T> var_set{v.begin(), v.end()};
    vector<T> result{var_set.begin(), var_set.end()};

    return result;
  }

  template<typename T, typename Fun>
  vector<T> filter(const vector<T>& v, Fun f) {

    vector<T> result;

    std::copy_if(v.begin(), v.end(), std::back_inserter(result), f);

    return result;
  }

  template<typename T, typename Fun>
  pair<vector<T>, vector<T>> bisect(const vector<T>& v, Fun f) {

    vector<T> sel; vector<T> rest;

    for (auto& e : v) {
      if (f(e)) { 
        sel.push_back(e);
      } else {
        rest.push_back(e);
      }
    }

    return {sel, rest};
  }

  static Lit with_min_val(const vector<Lit>& lits) {

    int cur_min = INT_MAX;
    const Lit* cur_lit = nullptr;

    for (auto& l : lits) {
      if (l.val < cur_min) {
        cur_min = l.val;
        cur_lit = &l;
      }
    }

    return *cur_lit;
  }

  static Lit with_max_val(const vector<Lit>& lits) {

    int cur_max = INT_MIN;
    const Lit* cur_lit = nullptr;

    for (auto& l : lits) {
      if (l.val > cur_max) {
        cur_max = l.val;
        cur_lit = &l;
      }
    }

    return *cur_lit;
  }

  /// assumes the same var
  /// a>=1 \/ a>=3  ->  a>=1
  static vector<Lit> apply_ge_rule(const vector<Lit>& lits) {
    vector<Lit> ge_lits, rest_op;
    std::tie(ge_lits, rest_op) = bisect(lits, [] (const Lit& l) { return l.op == ">="; });

    if (ge_lits.size() == 0) return lits;

    const auto min_lit = with_min_val(ge_lits);

    /// all other operators + one with ">="
    rest_op.push_back(min_lit);

    return rest_op;
  }

  /// assumes the same var
  /// a<=1 \/ a<=3  ->  a>=3
  static vector<Lit> apply_le_rule(const vector<Lit>& lits) {
    vector<Lit> le_lits, rest_op;
    std::tie(le_lits, rest_op) = bisect(lits, [] (const Lit& l) { return l.op == "<="; });

    if (le_lits.size() == 0) return lits;

    const auto max_lit = with_max_val(le_lits);

    /// all other operators + one with "<="
    rest_op.push_back(max_lit);

    return rest_op;
  }

  /// assumes the same var
  /// a!=1 \/ a<=3  ->  a!=3
  static vector<Lit> apply_ne_rule(const vector<Lit>& lits) {

    const auto ne_lits = filter(lits, [] (const Lit& l) { return l.op == "!="; });

    if (ne_lits.size() != 1) return lits;

    return {ne_lits[0]};
  }

  /// assumes the same var
  /// a>=3 a<=1  ->  a!=2
  static vector<Lit> apply_exclusion_rule(const vector<Lit>& lits) {

    const vector<Lit> le_lits = filter(lits, [] (const Lit& l) { return l.op == "<="; });
    const vector<Lit> ge_lits = filter(lits, [] (const Lit& l) { return l.op == ">="; });

    /// TODO(maxim): can also apply ne and ge rules here!

    if (le_lits.size() == 0 || ge_lits.size() == 0) return lits;

    const auto max_le = with_max_val(le_lits).val;
    const auto min_ge = with_min_val(ge_lits).val;

    if (min_ge - max_le == 2) {
      return {{le_lits[0].var, "!=", min_ge - 1, false, true}};
    }

    return lits;
  }

  vector<Lit> apply_rules_same_var(const string& var, const vector<Lit>& lits) {

    vector<Lit> selected_var; vector<Lit> result;
    std::tie(selected_var, result) = bisect(lits, [&var] (const Lit& l) { return l.var == var; });

    auto result_1 = apply_ne_rule(selected_var);
    auto result_2 = apply_ge_rule(result_1);
    auto result_3 = apply_le_rule(result_2);
    auto final = apply_exclusion_rule(result_3);

    result.insert(result.begin(), final.begin(), final.end());

    return result;
  }

  static string stringify_lit(const Lit& lit);

  static vector<Lit> split_to_lits(const string& ng_) {

    auto ng = ng_;

    trim(ng);

    auto str_lits = split(ng, ' ');

    auto lits = map<string, Lit>(str_lits, &parse_lit);

    return lits;

  }

  static string stringify_lit(const Lit& lit) {
    std::ostringstream ss;

    if(!lit.is_clean) return lit.var;

    ss << lit.var << lit.op;

    if (lit.is_bool) {
      ss << (lit.val == 0 ? "false" : "true");
    } else {
      ss << lit.val;
    }

    return ss.str();
  }

  string stringify_lits(const vector<Lit>& lits) {

    if (lits.size() == 0) return "";

    std::ostringstream ss;

    ss << stringify_lit(lits[0]);

    for (auto i = 1u; i < lits.size(); ++i) {
      ss << " " << stringify_lit(lits[i]);
    }

    return ss.str();
  }

  Lit negate_lit(const Lit& l) {
    auto result = l;
    result.op = negate_op(l.op);
    return result;
  }

  static bool should_negate(const Lit& l) {
    if (l.val == 0) {
      if (l.op == "<=" || l.op == "=") return true;
      return false;
    }

    if (l.val == 1 && l.op == "<") return true;
    return false;
  }

  static bool should_keep_as_is(const Lit& l) {
    if(!l.is_bool) return false;
    if (l.val == 1) {
      if (l.op == "=" || l.op == ">=") return true;
      return false;
    }

    if (l.val == 0 && l.op == ">") return true;
    return false;
  }

  static Lit simplify_expr_lit(const Lit& l) {

    if (should_negate(l)) {
      auto inner_lit = l.var.substr(1, l.var.size() - 2);
      auto negated = negate_lit(parse_lit(inner_lit));
      return negated;
    }

    if (should_keep_as_is(l)) {

      auto inner_lit = l.var.substr(1, l.var.size() - 2);
      auto mod = parse_lit(inner_lit);
      return mod;
    }

#ifdef MAXIM_DEBUG
    std::cerr << "can't simplify: " << l << endl;
#endif

    return l;

  }

  static vector<Lit> simplify_expressions_in_ng(const vector<Lit>& lits) {

    vector<Lit> result;

    for (auto& l : lits) {
      if (l.var[0] == '\'') {
        auto simple = simplify_expr_lit(l);
        result.push_back(simple);
      } else {
        result.push_back(l);
      }
    }

    return result;

  }

  string remove_redundant_wspaces(const string& ng) {

    auto parts = split(ng, ' ');

    bool inside = false;
    auto idx = 0u;

    string result{""};

    while (idx < parts.size()) {
      if (parts[idx][0] == '\'') {
        inside = true;
      }

      if (inside && (parts[idx].find('\'', 1) != string::npos)) {
        inside = false;
      }

      if (inside) {
        result += parts[idx];
        idx++;
      } else {
        result += parts[idx] + " ";
        idx++;
      }
    }

    return result;
  }

  string simplify_ng(const string& ng) {

    auto cleaned = remove_redundant_wspaces(ng);

    auto all_lits = split_to_lits(cleaned);

    auto lits = filter(all_lits, [](const Lit&l ){return l.is_clean;});
    auto dirty_lits = filter(all_lits, [](const Lit&l ){return !l.is_clean;});

    lits = simplify_expressions_in_ng(lits);

    /// try to simplify the exression

    const auto vars = map<Lit, string>(lits, [] (const Lit& l) { return l.var; });

    const auto uniq_vars = uniq(vars);

    // std::cout << "lits before: " << lits.size << endl;

    std::for_each(uniq_vars.begin(), uniq_vars.end(), [&lits](const string& var) {

      lits = apply_rules_same_var(var, lits);

    });

    lits.insert(lits.end(), dirty_lits.begin(), dirty_lits.end());
    std::sort(lits.begin(), lits.end(), [] (const Lit& lhs, const Lit& rhs) { return lhs.var < rhs.var; });

    auto res = stringify_lits(lits);

    return res;

  }

  static void performance_test() {

    perfHelper.begin("simplify literals");

    // currently takes ~58ms for 1000 nogoods

    const std::vector<string> lits {
      "X_INTRODUCED_60_=false X_INTRODUCED_3_!=1 X_INTRODUCED_3_<=0",
      "X_INTRODUCED_3_<=0 X_INTRODUCED_72_>=1 X_INTRODUCED_74_>=1 X_INTRODUCED_75_>=1 X_INTRODUCED_3_>=2 X_INTRODUCED_603_=false",
      "X_INTRODUCED_61_=false X_INTRODUCED_71_>=1 X_INTRODUCED_185_>=1 X_INTRODUCED_4_>=5 X_INTRODUCED_4_<=2 X_INTRODUCED_164_>=1 X_INTRODUCED_162_>=1 X_INTRODUCED_163_>=1 X_INTRODUCED_389_>=1 X_INTRODUCED_390_>=1 X_INTRODUCED_388_>=1 X_INTRODUCED_359_=true X_INTRODUCED_352_=true X_INTRODUCED_119_>=1 X_INTRODUCED_120_>=1 X_INTRODUCED_117_>=1 X_INTRODUCED_118_>=1 X_INTRODUCED_116_>=1 X_INTRODUCED_115_>=1 X_INTRODUCED_123_>=1 X_INTRODUCED_96_>=1 X_INTRODUCED_94_>=1 X_INTRODUCED_93_>=1 X_INTRODUCED_95_>=1 X_INTRODUCED_97_>=1 X_INTRODUCED_98_>=1 X_INTRODUCED_296_<=0 X_INTRODUCED_353_=true X_INTRODUCED_249_>=1 X_INTRODUCED_250_>=1 X_INTRODUCED_251_>=1 X_INTRODUCED_252_>=1 X_INTRODUCED_204_>=1 X_INTRODUCED_206_>=1 X_INTRODUCED_207_>=1 objective>=210 X_INTRODUCED_75_>=1 X_INTRODUCED_2_>=3 X_INTRODUCED_74_>=1 X_INTRODUCED_3_>=5 objective>=220 X_INTRODUCED_4_<=0 X_INTRODUCED_184_>=1 X_INTRODUCED_5_>=5 X_INTRODUCED_73_>=1 X_INTRODUCED_5_>=4",
      "X_INTRODUCED_2_<=0 X_INTRODUCED_117_>=1 X_INTRODUCED_97_>=1 X_INTRODUCED_120_>=1 X_INTRODUCED_98_>=1 X_INTRODUCED_669_=false X_INTRODUCED_119_>=1 X_INTRODUCED_5_>=5 X_INTRODUCED_95_>=1 X_INTRODUCED_94_>=1 X_INTRODUCED_71_>=1 X_INTRODUCED_74_>=1 X_INTRODUCED_75_>=1 X_INTRODUCED_5_<=0",
      "X_INTRODUCED_5_>=3 X_INTRODUCED_97_>=1 X_INTRODUCED_95_>=1 X_INTRODUCED_5_<=1 X_INTRODUCED_98_>=1 X_INTRODUCED_94_>=1 X_INTRODUCED_669_=false",
      "X_INTRODUCED_2_<=5 X_INTRODUCED_44_<=0 X_INTRODUCED_187_>=1 objective>=320 X_INTRODUCED_183_>=1 X_INTRODUCED_2_>=7",
      "X_INTRODUCED_2_>=2 X_INTRODUCED_75_>=1 X_INTRODUCED_74_>=1 X_INTRODUCED_2_<=0 X_INTRODUCED_72_>=1",
      "X_INTRODUCED_5_>=2 X_INTRODUCED_617_=false X_INTRODUCED_71_>=1 X_INTRODUCED_74_>=1 X_INTRODUCED_75_>=1 X_INTRODUCED_669_=false X_INTRODUCED_5_<=0",
      "X_INTRODUCED_5_>=2 X_INTRODUCED_72_>=1 X_INTRODUCED_669_=false X_INTRODUCED_71_>=1 X_INTRODUCED_74_>=1 X_INTRODUCED_75_>=1 X_INTRODUCED_5_<=0",
      "X_INTRODUCED_1_>=3 X_INTRODUCED_1_<=1 X_INTRODUCED_97_>=1 X_INTRODUCED_95_>=1 X_INTRODUCED_98_>=1 X_INTRODUCED_101_>=1 X_INTRODUCED_96_>=1 X_INTRODUCED_94_>=1 X_INTRODUCED_93_>=1"
    };

    for (auto i = 0u; i<1000u; ++i) {
      for (auto& l : lits) {
        simplify_ng(l);
      }
    }

    perfHelper.end();

  }


  void test_module() {

    test_simplify_ng();

    performance_test();


  }

  static void test_simplify_ng() {

    const std::vector<pair<string, string>> samples {
      {"'x[11]!=11'=false", "x[11]=11"},
      {"x>=7 x>=8 x<=2", "x>=7 x<=2"},
      {"z!=2 y>=8 y>=5 z>=4", "y>=5 z!=2"},
      {"how[4]<=2 'how[2] = -3'>=1", "how[2]=-3 how[4]<=2"},
      {"a=false b!=1 b<=0", "a=false b!=1"},
      {"a>=3 a<=1", "a!=2"},
      {"how[4]!=3 how[4]<=2 'how[2] = -3'>=1 'how[3] = -3'>=1 'how[4] = -3'>=1 'how[1] = -3'>=1",
       "how[1]=-3 how[2]=-3 how[3]=-3 how[4]!=3"}
    };

    for (auto& s: samples) {
      const auto& simplified = simplify_ng(s.first);
      if (simplified == s.second) {
        std::cerr << "test passed!\n";
      } else {
        std::cerr << "test did NOT pass! (expexted: " << s.second << " given: " << simplified << ")\n";
      }
    }

  }

  static void test_parse_lit() {

    const std::vector<pair<string, Lit>> samples {
      {"x>=5", Lit{"x", ">=", 5, false}},
      {"x<7", Lit{"x", "<", 7, false}},
      {"X_INT!=12", Lit{"X_INT", "!=", 12, false}},
    };

    for (auto& s : samples) {
      if (parse_lit(s.first) == s.second) {
        qDebug() << "test passed!";
      } else {
        qDebug() << "test did NOT pass!";
      }
    }

  }



}}


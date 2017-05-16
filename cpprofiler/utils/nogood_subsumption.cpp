#include "cpprofiler/utils/nogood_subsumption.hh"

#include <QString>
#include <QStringList>

#include <iostream>

using std::string;

namespace utils {

inline string clauseToString(const Clause& clause) {
  QStringList clause_string;
  for(const Lit& lid : clause)
    clause_string << QString::fromStdString(lid.var) +
                     QString::fromStdString(lid.op) +
                     QString::number(lid.val);
  clause_string.sort();
  return clause_string.join(" ").toStdString();
}

void SubsumptionFinder::populateClauses(const std::unordered_map<int, string>& sid2nogood,
                                        const std::vector<int64_t>& pool) {
  clauses.resize(pool.size());
  size_t top = 0;
  for(int64_t jsid : pool) {
    const QString qclause = QString::fromStdString(sid2nogood.at(static_cast<int>(jsid)));
    if(!qclause.isEmpty()) {
      Clause* clause = &clauses[top++];
      sid2clause[jsid] = clause;
      const QStringList lits = qclause.split(" ", QString::SplitBehavior::SkipEmptyParts);
      for(const QString& lit : lits)
        clause->push_back(lits::parse_lit(lit.toStdString()));
      std::sort(clause->begin(), clause->end());

      ordered_sids[static_cast<int>(clause->size())].push_back(jsid);
    }
  }
}

SubsumptionFinder::SubsumptionFinder(const std::unordered_map<int, string>& sid2nogood,
                                     const std::vector<int64_t>& pool) {
  populateClauses(sid2nogood, pool);
}

SubsumptionFinder::SubsumptionFinder(const std::unordered_map<int, string>& sid2nogood) {
  std::vector<int64_t> all;
  for(auto& sidNogood : sid2nogood) {
    if(!sidNogood.second.empty())
      all.push_back(sidNogood.first);
  }
  populateClauses(sid2nogood, all);
}

inline
bool subsumes(const Lit& a, const Lit& b) {
  if(a.var == b.var) {
    const std::vector<Lit> lits {a, b};
    const std::vector<Lit> result = apply_rules_same_var(a.var, lits);
    return result.size() == 1 && result[0] == a;
  }
  return false;
}

inline
bool isSubset(const Clause& a, const Clause& b) {
  if(a.size() > b.size()) return false;
  size_t i=0;
  for(const Lit& lj : b) {
    if(subsumes(a[i], lj)) {
      i++;
      if(i == a.size()) return true;
    }
  }
  return false;
}

const Clause* SubsumptionFinder::findSubsumingClause(const Clause& iclause) const {
  for(const auto& size_sids : ordered_sids) {
    if(static_cast<size_t>(size_sids.first) >= iclause.size()) break;
    for(int64_t jsid : size_sids.second) {
      const Clause* jclause = sid2clause.at(jsid);
      if(isSubset(*jclause, iclause))
        return jclause;
    }
  }
  return &iclause;
}

string SubsumptionFinder::getSubsumingClauseString(int64_t sid) const {
  const Clause* iclause = sid2clause.at(sid);
  const Clause* subsuming_clause = findSubsumingClause(*iclause);
  return clauseToString(*subsuming_clause);
}

}

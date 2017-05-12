#include "cpprofiler/utils/nogood_subsumption.hh"

namespace Utils {

SubsumptionFinder::SubsumptionFinder(const std::unordered_map<int, std::string>& sid2nogood,
                                     const std::vector<int64_t>& pool) {
  clauses.reserve(pool.size());
  for(int64_t jsid : pool) {
    const QString qclause = QString::fromStdString(sid2nogood.at(jsid));
    if(!qclause.isEmpty()) {
      clauses.push_back(QVector<int>());
      QVector<int>* clause = &clauses.back();
      sid2clause[jsid] = clause;
      const QStringList lits = qclause.split(" ", QString::SplitBehavior::SkipEmptyParts);
      for(const QString& lit : lits) {
        int id;
        auto id_it = lit2id.find(lit);
        if(id_it == lit2id.end()) {
          id = lit2id.size();
          lit2id[lit] = id;
          id2lit[id] = lit;
        } else {
          id = *id_it;
        }
        clause->append(id);
      }
      std::sort(clause->begin(), clause->end());

      ordered_sids[clause->size()].append(jsid);
    }
  }
}

inline
bool isSubset(const QVector<int>& a, const QVector<int>& b, int i_lit=-1) {
  if(a.size() - (i_lit!=-1) > b.size()) return false;
  int i=0;
  for(int j : b) {
    if(a[i] == i_lit) {
      i++;
      if(i == a.size()) return true;
    }
    if(a[i] < j) {
      return false;
    } else if(a[i] == j) {
      i++;
      if(i == a.size()) return true;
    }
  }
  return false;
}

const QVector<int>* SubsumptionFinder::findSubsumingClause(const QVector<int>& iclause) const {
  for(const auto& size_sids : ordered_sids) {
    if(size_sids.first >= iclause.size()) break;
    for(int64_t jsid : size_sids.second) {
      const QVector<int>* jclause = sid2clause[jsid];
      if (isSubset(*jclause, iclause))
        return jclause;
    }
  }
  return &iclause;
}

inline
QString SubsumptionFinder::clauseToString(const QVector<int>& clause) const {
  QStringList clause_string;
  for(int lid : clause)
    clause_string << id2lit[lid];
  return clause_string.join(" ");
}

QString SubsumptionFinder::getSubsumingClauseString(int64_t sid) const {
  const QVector<int>* iclause = sid2clause[sid];
  const QVector<int>* subsuming_clause = findSubsumingClause(*iclause);
  return clauseToString(*subsuming_clause);
}

}

#ifndef NOGOOD_SUBSUMPTION_HH
#define NOGOOD_SUBSUMPTION_HH

#include <QHash>
#include <QVector>

#include <vector>
#include <cinttypes>
#include <map>
#include <unordered_map>

#include "cpprofiler/utils/literals.hh"

using Lit = utils::lits::Lit;
using Clause = QVector<const Lit*>;

class Execution;
namespace utils {
class SubsumptionFinder {
public:
  SubsumptionFinder(const std::unordered_map<int, std::string>& sid2nogood,
                    const std::vector<int64_t>& pool);
  QString getSubsumingClauseString(int64_t sid) const;

private:
  QString clauseToString(const Clause& clause) const;
  const Clause* findSubsumingClause(const Clause& iclause) const;

  QVector<Clause> clauses;
  QHash<int64_t, Clause*> sid2clause;
  std::map<int, QVector<int64_t> > ordered_sids;

  QVector<Lit> literals;
  QHash<QString, const Lit*> lit2id;
  QHash<const Lit*, QString> id2lit;
};
}

#endif // NOGOOD_SUBSUMPTION_HH

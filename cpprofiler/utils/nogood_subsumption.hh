#ifndef NOGOOD_SUBSUMPTION_HH
#define NOGOOD_SUBSUMPTION_HH

#include "cpprofiler/utils/literals.hh"

#include <vector>
#include <map>
#include <unordered_map>
#include <cinttypes>

class Execution;

namespace utils {

using Lit = utils::lits::Lit;
using Clause = std::vector<Lit>;

class SubsumptionFinder {
public:
  SubsumptionFinder(const std::unordered_map<int, std::string>& sid2nogood,
                    const std::vector<int64_t>& pool);
  SubsumptionFinder(const std::unordered_map<int, std::string>& sid2nogood);
  std::string getSubsumingClauseString(int64_t sid) const;

private:
  const Clause* findSubsumingClause(const Clause& iclause) const;

  void populateClauses(const std::unordered_map<int, std::string>& sid2nogood,
                       const std::vector<int64_t>& pool);

  std::vector<Clause> clauses;
  std::unordered_map<int64_t, Clause*> sid2clause;
  std::map<int, std::vector<int64_t> > ordered_sids;
};

}

#endif // NOGOOD_SUBSUMPTION_HH

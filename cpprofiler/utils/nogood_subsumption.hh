#ifndef NOGOOD_SUBSUMPTION_HH
#define NOGOOD_SUBSUMPTION_HH

#include "cpprofiler/utils/literals.hh"

#include <vector>
#include <map>
#include <unordered_map>
#include <cinttypes>
#include <nogood_representation.hh>

class Execution;

namespace utils { namespace subsum {

void test_module();

class SubsumptionFinder {
  using Lit = utils::lits::Lit;
  using Clause = std::vector<Lit>;
public:
  SubsumptionFinder(const Sid2Nogood& sid2nogood,
                    const std::vector<int64_t>& pool,
                    bool renamed,
                    bool simplified);

  /// Use all nogoods for 'pool'
  SubsumptionFinder(const Sid2Nogood& sid2nogood,
                    bool renamed,
                    bool simplified);

  int64_t getSubsumingClauseString(int64_t sid,
                                   bool filter_only_earlier_sids = true) const;

  struct SSRResult {
      std::string newNogood;
      std::vector<int64_t> sids;
  };
  SSRResult getSelfSubsumingResolutionString(int64_t sid,
                                             bool filter_only_earlier_sids = true) const;

private:
  const Clause* findSubsumingClause(const Clause& iclause, bool filter_only_earlier_sids) const;

  void populateClauses(const Sid2Nogood& sid2nogood,
                       const std::vector<int64_t>& pool,
                       bool renamed, bool simplified);

  std::vector<Clause> clauses;
  std::unordered_map<int64_t, Clause*> sid2clause;
  std::map<int, std::vector<int64_t>> ordered_sids;
  bool _renamed {false};
  bool _simplified {false};
};

}}

#endif // NOGOOD_SUBSUMPTION_HH

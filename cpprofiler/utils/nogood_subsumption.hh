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
  SubsumptionFinder(const Uid2Nogood& uid2nogood,
                    const std::vector<NodeUID>& pool,
                    bool renamed,
                    bool simplified);

  /// Use all nogoods for 'pool'
  SubsumptionFinder(const Uid2Nogood& uid2nogood,
                    bool renamed,
                    bool simplified);

  NodeUID getSubsumingClauseString(NodeUID uid,
                                   bool filter_only_earlier_uids = true) const;

  struct SSRResult {
      std::string newNogood;
      std::vector<NodeUID> uids;
  };
  SSRResult getSelfSubsumingResolutionString(NodeUID uid,
                                             bool filter_only_earlier_sids = true) const;

private:
  const Clause* findSubsumingClause(const Clause& iclause, bool filter_only_earlier_sids) const;

  void populateClauses(const Uid2Nogood& uid2nogood,
                       const std::vector<NodeUID>& pool,
                       bool renamed, bool simplified);

  std::vector<Clause> clauses;
  std::unordered_map<NodeUID, Clause*> uid2clause;
  std::map<int, std::vector<NodeUID>> ordered_uids;
  bool _renamed {false};
  bool _simplified {false};
};

}}

#endif // NOGOOD_SUBSUMPTION_HH

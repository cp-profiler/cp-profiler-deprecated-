#include "cpprofiler/utils/nogood_subsumption.hh"
#include "cpprofiler/utils/string_utils.hh"

#include <QDebug>
#include <iostream>
#include <algorithm>
#include <nogood_representation.hh>

using std::string;
using std::vector;
using std::map;

namespace utils { namespace subsum {

using Lit = utils::lits::Lit;
using Clause = std::vector<Lit>;

void SubsumptionFinder::populateClauses(const Uid2Nogood& uid2nogood,
                                        const std::vector<NodeUID>& pool,
                                        bool renamed, bool simplified) {
  clauses.resize(pool.size());
  size_t top = 0;
  for(NodeUID uid : pool) {
    auto& ng = uid2nogood.at(uid);
    const string* sclause;
    if(renamed) {
      sclause = simplified ? &ng.simplified : &ng.renamed;
    } else {
      sclause = &ng.original;
    }
    if(!sclause->empty()) {
      Clause* clause = &clauses[top++];
      uid2clause[uid] = clause;
      const vector<string> lits = utils::split(*sclause, ' ');
      for(const string& lit : lits)
        clause->push_back(lits::parse_lit(lit));
      std::sort(clause->begin(), clause->end());

      ordered_uids[static_cast<int>(clause->size())].push_back(uid);
    }
  }
}

SubsumptionFinder::SubsumptionFinder(const Uid2Nogood& uid2nogood,
                                     const std::vector<NodeUID>& pool,
                                     bool renamed, bool simplified) {
  populateClauses(uid2nogood, pool, renamed, simplified);
}

SubsumptionFinder::SubsumptionFinder(const Uid2Nogood& uid2nogood,
                                     bool renamed, bool simplified) {
  std::vector<NodeUID> all;
  for(auto& uidNogood : uid2nogood) {
    if(!uidNogood.second.original.empty())
      all.push_back(uidNogood.first);
  }
  populateClauses(uid2nogood, all, renamed, simplified);
}

inline
bool subsumes(const Lit& a, const Lit& b) {
  if(a==b) return true;
  if(a.var != b.var) return false;
  if(a.op[0] != b.op[0]) return false;
  if(a.op[0] == '!' || a.op[0] == '=') return false;

  int aval = a.val;
  int bval = b.val;
  if(a.op[0] == '<') {
    if(a.op.size() == 1) aval--;
    if(b.op.size() == 1) bval--;
    return aval <= bval;
  } else if(a.op[0] == '>') {
    if(a.op.size() == 1) aval++;
    if(b.op.size() == 1) bval++;
    return aval >= bval;
  }
  return false;
}

inline
bool isSubset(const Clause& a, const Clause& b, const Lit* const ignore = nullptr) {
  if(a.size() > b.size()) return false;
  size_t i=0;
  for(const Lit& lj : b) {
    if((ignore && a[i]==*ignore) || subsumes(a[i], lj)) {
      i++;
      if(i == a.size()) return true;
    }
  }
  return false;
}

inline
bool contains(const Clause& c, const Lit& lit) {
  for(const Lit& l : c)
      if(l == lit) return true;
  return false;
}

SubsumptionFinder::SSRResult SubsumptionFinder::getSelfSubsumingResolutionString(NodeUID uid,
                                                                                 bool filter_only_earlier_sids) const {
  SSRResult r;
  const Clause* iclause = uid2clause.at(uid);
  vector<bool> rem_lit(iclause->size(), false);
  for(const auto& size_uids : ordered_uids) {
    if(static_cast<size_t>(size_uids.first) >= iclause->size()) break;
    for(size_t i=0; i<iclause->size(); i++) {
      const Lit& lit = (*iclause)[i];
      const Lit negLit = lits::negate_lit(lit);
      for(NodeUID juid : size_uids.second) {
        if(!filter_only_earlier_sids || juid < uid) {
          const Clause* jclause = uid2clause.at(juid);
          if(contains(*jclause, negLit) &&
                  isSubset(*jclause, *iclause, &negLit)) {
            r.uids.push_back(juid);
            rem_lit[i] = true;
            goto lit_removed;
          }
        }
      }
      lit_removed:;
    }
  }
  Clause newClause;
  for(size_t i=0; i<iclause->size(); i++)
    if(!rem_lit[i]) newClause.push_back((*iclause)[i]);
  r.newNogood = lits::stringify_lits(newClause);
  return r;
}

NodeUID SubsumptionFinder::getSubsumingClauseString(NodeUID uid,
                                                    bool filter_only_earlier_uids) const {
  const Clause* iclause = uid2clause.at(uid);
  for(const auto& size_uids : ordered_uids) {
    if(static_cast<size_t>(size_uids.first) >= iclause->size()) break;
    for(NodeUID juid : size_uids.second) {
      if(!filter_only_earlier_uids || juid < uid) {
        const Clause* jclause = uid2clause.at(juid);
        if(isSubset(*jclause, *iclause))
          return juid;
      }
    }
  }
  return uid;
}

static void test_subsumption();
static void test_resolution();

void test_module() {
    test_subsumption();
    test_resolution();
}

static void test_subsumption() {
  vector<std::pair<NodeUID, NogoodViews> > testNogoods {
    std::make_pair(NodeUID{1, -1, -1}, NogoodViews("b>2 c<=3")),
    std::make_pair(NodeUID{2, -1, -1}, NogoodViews("a<=1 b<=2 c<=3")),
  };

  Uid2Nogood uid2nogood;
  uid2nogood.insert(testNogoods.begin(), testNogoods.end());

  utils::subsum::SubsumptionFinder sf(uid2nogood, false, false);

  int count=0;
  for(auto& sn : testNogoods)
    if(sf.getSubsumingClauseString(sn.first) == testNogoods[0].first)
      count++;

  qDebug() << count << "/" << testNogoods.size() << " Subsumption tests passed";
}

static void test_resolution() {
  vector<std::pair<NodeUID, NogoodViews> > testNogoods {
    std::make_pair(NodeUID{1, -1, -1}, NogoodViews("b>2 c<=3")),
    std::make_pair(NodeUID{2, -1, -1}, NogoodViews("a<=1 b<=2 c<=3")),
  };

  Uid2Nogood uid2nogood;
  uid2nogood.insert(testNogoods.begin(), testNogoods.end());

  utils::subsum::SubsumptionFinder sf(uid2nogood, false, false);

  int count=0;
  SubsumptionFinder::SSRResult r1 = sf.getSelfSubsumingResolutionString({1, -1, -1});
  if(r1.newNogood == "b>2 c<=3" && r1.uids.size() == 0)
    count++;

  SubsumptionFinder::SSRResult r2 = sf.getSelfSubsumingResolutionString({2, -1, -1});
  if(r2.newNogood == "a<=1 c<=3" && r2.uids.size() == 1 && r2.uids[0] == NodeUID{1, -1, -1})
    count++;

  qDebug() << count << "/" << testNogoods.size() << " Self-subsuming resolution tests passed";
}

}}

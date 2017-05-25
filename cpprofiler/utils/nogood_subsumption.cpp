#include "cpprofiler/utils/nogood_subsumption.hh"
#include "cpprofiler/utils/string_utils.hh"

#include <QDebug>
#include <iostream>
#include <algorithm>
#include <nogood_representation.h>

using std::string;
using std::vector;
using std::map;

namespace utils { namespace subsum {

using Lit = utils::lits::Lit;
using Clause = std::vector<Lit>;

void SubsumptionFinder::populateClauses(const Sid2Nogood& sid2nogood,
                                        const std::vector<int64_t>& pool,
                                        bool renamed, bool simplified) {
  clauses.resize(pool.size());
  size_t top = 0;
  for(int64_t jsid : pool) {
    auto& ng = sid2nogood.at(static_cast<int>(jsid));
    const string* sclause;
    if(renamed) {
      sclause = simplified ? &ng.simplified : &ng.renamed;
    } else {
      sclause = &ng.original;
    }
    if(!sclause->empty()) {
      Clause* clause = &clauses[top++];
      sid2clause[jsid] = clause;
      const vector<string> lits = utils::split(*sclause, ' ');
      for(const string& lit : lits)
        clause->push_back(lits::parse_lit(lit));
      std::sort(clause->begin(), clause->end());

      ordered_sids[static_cast<int>(clause->size())].push_back(jsid);
    }
  }
}

SubsumptionFinder::SubsumptionFinder(const Sid2Nogood& sid2nogood,
                                     const std::vector<int64_t>& pool,
                                     bool renamed, bool simplified) {
  populateClauses(sid2nogood, pool, renamed, simplified);
}

SubsumptionFinder::SubsumptionFinder(const Sid2Nogood& sid2nogood,
                                     bool renamed, bool simplified) {
  std::vector<int64_t> all;
  for(auto& sidNogood : sid2nogood) {
    if(!sidNogood.second.original.empty())
      all.push_back(sidNogood.first);
  }
  populateClauses(sid2nogood, all, renamed, simplified);
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

string SubsumptionFinder::getSelfSubsumingResolutionString(int64_t sid,
                                     bool filter_only_earlier_sids) const {
  const Clause* iclause = sid2clause.at(sid);
  vector<bool> rem_lit(iclause->size(), false);
  for(const auto& size_sids : ordered_sids) {
    if(static_cast<size_t>(size_sids.first) >= iclause->size()) break;
    for(size_t i=0; i<iclause->size(); i++) {
      const Lit& lit = (*iclause)[i];
      const Lit negLit = lits::negate_lit(lit);
      for(int64_t jsid : size_sids.second) {
        if(!filter_only_earlier_sids || jsid < sid) {
          const Clause* jclause = sid2clause.at(jsid);
          if(contains(*jclause, negLit) &&
                  isSubset(*jclause, *iclause, &negLit)) {
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
    //else std::cerr << "Removed: " << (*iclause)[i].var << (*iclause)[i].op << (*iclause)[i].val
    //               << " from " << sid << "\n";
  return lits::stringify_lits(newClause);
}

string SubsumptionFinder::getSubsumingClauseString(int64_t sid,
                                                   bool filter_only_earlier_sids) const {
  const Clause* iclause = sid2clause.at(sid);
  for(const auto& size_sids : ordered_sids) {
    if(static_cast<size_t>(size_sids.first) >= iclause->size()) break;
    for(int64_t jsid : size_sids.second) {
      if(!filter_only_earlier_sids || jsid < sid) {
        const Clause* jclause = sid2clause.at(jsid);
        if(isSubset(*jclause, *iclause))
          return lits::stringify_lits(*jclause);
      }
    }
  }
  return lits::stringify_lits(*iclause);
}

static void test_subsumption();
static void test_resolution();

void test_module() {
    test_subsumption();
    test_resolution();
}

static void test_subsumption() {
  vector<std::pair<int64_t, NogoodViews> > testNogoods {
    std::make_pair(1, NogoodViews("b<=2 c<=3")),
    std::make_pair(2, NogoodViews("a<=1 b<=2 c<=3")),
  };

  Sid2Nogood sid2nogood;
  sid2nogood.insert(testNogoods.begin(), testNogoods.end());

  utils::subsum::SubsumptionFinder sf(sid2nogood, false, false);

  int count=0;
  for(auto& sn : testNogoods)
    if(sf.getSubsumingClauseString(sn.first) == testNogoods[0].second.original)
      count++;

  qDebug() << count << "/" << testNogoods.size() << " Subsumption tests passed";
}

static void test_resolution() {
  vector<std::pair<int, NogoodViews> > testNogoods {
    std::make_pair(1, NogoodViews("b>2 c<=3")),
    std::make_pair(2, NogoodViews("a<=1 b<=2 c<=3")),
  };

  Sid2Nogood sid2nogood;
  sid2nogood.insert(testNogoods.begin(), testNogoods.end());

  utils::subsum::SubsumptionFinder sf(sid2nogood, false, false);

  int count=0;
  if(sf.getSelfSubsumingResolutionString(1) == "b>2 c<=3")
    count++;
  if(sf.getSelfSubsumingResolutionString(2) == "a<=1 c<=3")
    count++;

  qDebug() << count << "/" << testNogoods.size() << " self-subsuming resolution tests passed";
}

}}

#ifndef NOGOOD_SUBSUMPTION_HH
#define NOGOOD_SUBSUMPTION_HH

#include <vector>
#include <cinttypes>
#include <qhash.h>
#include <map>

class Execution;
namespace Utils {
class SubsumptionFinder {
public:
  SubsumptionFinder(const Execution& e, const std::vector<int64_t>& pool);
  QString getSubsumingClauseString(int64_t sid);

private:
  QString clauseToString(const QVector<int>& clause);
  const QVector<int>& findSubsumingClause(const QVector<int>& iclause);

  const Execution& _execution;
  QHash<int64_t, QVector<int> > sid2clause;
  std::map<int, QVector<int64_t> > ordered_sids;

  QHash<QString, int> lit2id;
  QHash<int, QString> id2lit;
};
}

#endif // NOGOOD_SUBSUMPTION_HH

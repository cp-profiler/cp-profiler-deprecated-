#ifndef NOGOOD_SUBSUMPTION_HH
#define NOGOOD_SUBSUMPTION_HH

#include <qvector.h>
#include <vector>
#include <cinttypes>
#include <qhash.h>
#include <map>
#include <unordered_map>

class Execution;
namespace Utils {
class SubsumptionFinder {
public:
  SubsumptionFinder(const std::unordered_map<int, std::string>& sid2nogood,
                    const std::vector<int64_t>& pool);
  QString getSubsumingClauseString(int64_t sid) const;

private:
  QString clauseToString(const QVector<int>& clause) const;
  const QVector<int>* findSubsumingClause(const QVector<int>& iclause) const;

  QVector<QVector<int> > clauses;
  QHash<int64_t, QVector<int>* > sid2clause;
  std::map<int, QVector<int64_t> > ordered_sids;

  QHash<QString, int> lit2id;
  QHash<int, QString> id2lit;
};
}

#endif // NOGOOD_SUBSUMPTION_HH

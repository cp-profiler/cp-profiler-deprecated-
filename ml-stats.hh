#ifndef ML_STATS_HH
#define ML_STATS_HH

#include "execution.hh"
#include "visualnode.hh"

void collectMLStats(VisualNode* root, const NodeAllocator& na, Execution* execution, std::ostream& out = std::cout);

#endif

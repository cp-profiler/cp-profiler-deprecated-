#ifndef ML_STATS_HH
#define ML_STATS_HH

#include "execution.hh"
#include "visualnode.hh"

void collectMLStats(VisualNode* root, const VisualNode::NodeAllocator& na, Execution* execution);

#endif

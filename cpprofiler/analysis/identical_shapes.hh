#pragma once

using GroupsOfNodes_t = std::vector<std::vector<VisualNode*>>;

namespace cpprofiler {
namespace analysis {
namespace subtrees {


GroupsOfNodes_t findIdentical(NodeTree& nt);
// GroupsOfNodes_t findIdentical(Execution& ex);


}
}
}
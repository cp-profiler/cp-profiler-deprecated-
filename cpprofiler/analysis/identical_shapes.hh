#pragma once

using GroupsOfNodes_t = std::vector<std::vector<VisualNode*>>;

class Execution;

namespace cpprofiler {
namespace analysis {
namespace subtrees {


GroupsOfNodes_t findIdentical(Execution& nt);


}
}
}
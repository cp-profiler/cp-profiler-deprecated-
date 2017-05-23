#include "tests.hh"

#include "cpprofiler/utils/literals.hh"
#include "cpprofiler/utils/nogood_subsumption.hh"


namespace cpprofiler {
namespace unit_tests {

  void run() {

    utils::lits::test_module();
    utils::subsum::test_module();

  }

}}


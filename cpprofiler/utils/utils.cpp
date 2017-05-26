#include "utils.hh"

#include <sstream>
#include <random>

namespace utils {

  int randInt(int low, int high) {
    static std::random_device rdev;
    static std::default_random_engine re(rdev());
    std::uniform_int_distribution<int> uniform_dist{low, high};
    int value = uniform_dist(re);
    return value;
  }

  bool randBool() {
    int value = randInt(0, 1);
    return static_cast<bool>(value);
  }

}

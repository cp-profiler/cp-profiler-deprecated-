#ifndef PATH_UTILS_HH
#define PATH_UTILS_HH

#include <vector>
#include <string>

namespace utils {

#define minor_sep '|'
#define major_sep ';'

struct PathPair {
    std::vector<std::string> model_level;
    std::vector<std::string> decomp_level;
};

PathPair getPathPair(const std::string& path,
                     bool omitDecomp = false);

}


#endif // PATH_UTILS_HH

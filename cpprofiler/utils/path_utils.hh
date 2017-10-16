#ifndef PATH_UTILS_HH
#define PATH_UTILS_HH

#include <vector>
#include <string>

namespace utils {

#define minor_sep '|'
#define major_sep ';'

std::vector<std::string> getPathHead(const std::string& path,
                                     bool leaveModel = false,
                                     bool includeTrail = false);


}


#endif // PATH_UTILS_HH

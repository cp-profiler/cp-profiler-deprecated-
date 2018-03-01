#include <string>
#include <vector>

#include "cpprofiler/utils/string_utils.hh"
#include "cpprofiler/utils/path_utils.hh"

using std::vector;
using std::string;

namespace utils {

PathPair getPathPair(const string& path,
                     bool omitDecomp) {
  PathPair ph;
  vector<string> pathSplit = utils::split(path, major_sep);

  if(pathSplit.size() == 0) return ph;

  string mzn_file;
  size_t i=0;
  do {
    string path_head = pathSplit[i];

    vector<string> head = utils::split(path_head, minor_sep);
    string head_file;
    if(head.size() > 0) {
      if(i==0) mzn_file = head[0];
      head_file = head[0];
    }
    if(head_file != mzn_file)
      break;

    ph.model_level.push_back(path_head);
    i++;
  } while(i < pathSplit.size());

  if(!omitDecomp) {
    for(;i<pathSplit.size();i++)
      ph.decomp_level.push_back(pathSplit[i]);
  }

  return ph;
}

}

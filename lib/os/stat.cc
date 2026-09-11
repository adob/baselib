import "lib/os/stdio.h";
#include "stat.h"



using namespace lib;
using namespace os;

// FileInfo os::stat(str path, error err) {
//     FileInfo fi;

//     int r = ::stat(path.c_str(), &fi.stat);
//     if (r) {
//         err(os::from_errno(errno));
//         return {};
//     }

//     return {};
// }
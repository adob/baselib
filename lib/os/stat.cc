#include "stat.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error.h"
#include "lib/os/file.h"

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
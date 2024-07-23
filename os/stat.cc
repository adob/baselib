#include "stat.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error.h"

using namespace lib;
using namespace os;

FileInfo os::stat(str path, error &err) {
    struct stat statbuf;

    int r = ::stat(path.c_str(), &statbuf);
    if (r) {
        err(os::from_errno(errno));
        return {};
    }

    return {};
}
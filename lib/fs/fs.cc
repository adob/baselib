#include "fs.h"

using namespace lib;
using namespace fs;

FileMode FileMode::perm(this FileMode m) {
    return m & ModePerm;
}
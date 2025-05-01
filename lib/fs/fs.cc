#include "fs.h"
#include "lib/io/io.h"

using namespace lib;
using namespace fs;

FileMode FileMode::perm(this FileMode m) {
    return m & ModePerm;
}

void PathError::fmt(io::Writer &out, error err) const {
    fmt::fprintf(out, err, "%s %s: %s", this->op, this->path, this->err);
}
#include "fs.h"

using namespace lib;
using namespace fs;

FileMode FileMode::perm(this FileMode m) {
    return m & ModePerm;
}

void PathError::describe(io::OStream &out) const {
    fmt::fprintf(out, "%s %s: %s", this->op, this->path, this->err);
}
#include "join.h"
#include "../base.h"
#include "../io.h"

using namespace lib;
using namespace errors;

void JoinError::fmt(io::Writer &out, error err) const {
    if (len(this->errs) == 0) {
        return;
    }

    this->errs[0]->fmt(out, err);

    for (const Error *e : this->errs+1) {
        out.write_byte('\n', err);;
        e->fmt(out, err);
    }
}

JoinError errors::join(view<Error*> errs) {
    return JoinError(errs);
}

view<Error*> JoinError::unwrap() const {
    return this->errs;
}
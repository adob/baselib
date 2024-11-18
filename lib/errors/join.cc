#include "join.h"
#include "../base.h"
#include "../io.h"

using namespace lib;
using namespace errors;

void JoinError::describe(io::OStream &out) const {
    if (len(this->errs) == 0) {
        return;
    }

    this->errs[0]->describe(out);


    for (const Error *err : this->errs+1) {
        out.write('\n', error::ignore);;
        err->describe(out);
    }
}

JoinError errors::join(view<Error*> errs) {
    return JoinError(errs);
}

view<Error*> JoinError::unwrap() const {
    return this->errs;
}
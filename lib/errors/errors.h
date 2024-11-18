#pragma once

#include "../error.h"
#include "../io/io_stream.h"

namespace lib::errors {
    BasicError create(str msg);
    bool is(Error const &err, TypeID target);
    Error const *as(Error const &err, TypeID target);

    struct ErrUnsupported : ErrorBase<ErrUnsupported, "unsupported operation"> {};
}
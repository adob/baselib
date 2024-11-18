#pragma once

#include "../error.h"
#include "../io/io_stream.h"

namespace lib::errors {
    struct JoinError : ErrorBase<JoinError> {
        view<Error*> errs;

        explicit JoinError(view<Error*> errs) : errs(errs) {}

        virtual void describe(io::OStream &out) const override;
        virtual view<Error*> unwrap() const override;
    } ;

    JoinError join(view<Error*>);
}
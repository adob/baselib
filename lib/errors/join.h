#pragma once

#include "../error.h"
#include "../io/io.h"

namespace lib::errors {
    struct JoinError : ErrorBase<JoinError> {
        view<Error*> errs;

        explicit JoinError(view<Error*> errs) : errs(errs) {}

        virtual void fmt(io::Writer &out, error err) const override;
        virtual view<Error*> unwrap() const override;
    } ;

    JoinError join(view<Error*>);
}
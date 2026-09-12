module;
#include "errors_impl.h"

export module lib.errors;
export import lib.str;
export import lib.type_id;
export import lib.types;
export import lib.error;


export extern "C++" {
namespace lib::errors {
    BasicError create(str msg);
    bool is(Error const &err, TypeID target);
    Error const *as(Error const &err, TypeID target);
    Error &cast(Error &err, TypeID target);

    struct ErrUnsupported : ErrorBase<ErrUnsupported, "unsupported operation"> {};

    void log_error(const Error &);

    struct WriterToError : Error {
        io::WriterTo &writer_to;

        WriterToError(io::WriterTo &w) : Error(type_id<WriterToError>), writer_to(w) {}

        virtual void fmt(io::Writer &out, error err) const override;
    } ;
}
}

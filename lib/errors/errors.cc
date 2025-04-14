#include "errors.h"
#include "../base.h"
#include "../io.h"
#include "lib/error.h"
#include "lib/print.h"

using namespace lib;
using namespace errors;


BasicError errors::create(str msg) {
  return BasicError(msg);
}

bool errors::is(Error const& err, TypeID target) {
    return as(err, target) != nil;
 }

Error const *errors::as(Error const &err, TypeID target) {
  if (err.type == target) {
      return &err;
  };

  //using is_method = bool (*)(const error *, TypeID);
  
  #if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wpmf-conversions"
    using IsMemberFunction = bool (Error::*)(TypeID) const;
    using IsFunction = bool (*)(const Error*, TypeID);

    IsMemberFunction is_pmf = &Error::is;
    IsFunction derived_is = (IsFunction) (err.*is_pmf);
    IsFunction base_is = (IsFunction) (IsMemberFunction) &Error::is;

    if (base_is != derived_is) {
      // print "calling derived_is";
      bool result = derived_is(&err, target);
      if (result) {
          return  &err;
      }
    }


  #pragma GCC diagnostic pop
  # elif defined(__clang__)
    panic("unimplemented");
  #endif

  // if (err.is(target)) {
  //     return &err;
  // }

  view<Error*> unwrapped;
#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wpmf-conversions"
    using UnwrapMemberFunction = view<Error*> (Error::*)() const;
    using UnwrapFunction = view<Error*> (*)(const Error*);

    UnwrapMemberFunction unwrap_pmf = &Error::unwrap;
    UnwrapFunction derived_unwrap = (UnwrapFunction) (err.*unwrap_pmf);
    UnwrapFunction base_unwrap = (UnwrapFunction) &Error::unwrap;

    if (base_unwrap != derived_unwrap) {
      unwrapped = derived_unwrap(&err);
      // print "unwrapped returned", unwrapped;
    }


  #pragma GCC diagnostic pop
# elif defined(__clang__)
  panic("unimplemented");
#endif
  
    // print "len", len(unwrapped);
    for (Error const *e : unwrapped) {
      // print "check", (intptr) target, (intptr) e->type;

      #if defined(__GNUC__) && !defined(__clang__)
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wpmf-conversions"
        using IsMemberFunction = bool (Error::*)(TypeID) const;
        using IsFunction = bool (*)(const Error*, TypeID);

        IsMemberFunction is_pmf = &Error::is;
        IsFunction derived_is = (IsFunction) (e->*is_pmf);
        IsFunction base_is = (IsFunction) (IsMemberFunction) &Error::is;

        if (base_is != derived_is) {
          // print "calling derived is", (intptr) base_is, (intptr) derived_is, (intptr) base_unwrap;
          bool b = derived_is(e, target);
          if (b) {
            return e;
          }
        } else {
          // print "calling default is";
          bool b = target == e->type;
          if (b) {
            return e;
          }
        }


      #pragma GCC diagnostic pop
      # else
      if (e->is(target)) {

          return e;
      }
      #endif

  }

  return nil;
}

void errors::log_error(const Error &e) {
  fmt::fprintf(stderr, "error: %v\n", e); 
}

void WriterToError::fmt(io::Writer &out, error err) const {
  this->writer_to.write_to(out, err);
}

Error &errors::cast(Error &err, TypeID target) {
  if (err.type != target) {
    panic("bad cast");
  }

  return err;
}

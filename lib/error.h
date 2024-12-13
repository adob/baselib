#pragma once

#include <functional>

#include "types.h"
#include "str.h"
#include "array.h"
#include "type_id.h"

namespace lib {

    struct error;
    
    namespace fmt {
        struct Fmt;
    }

    namespace io {
        struct OStream;
        struct WriterTo;
    }

    struct Error;
    struct ErrorReporter;

    void panic(const Error &e);

    namespace errors {
        bool is(Error const& err, TypeID target);
        Error const *as(Error const &err, TypeID target);
    }

    struct Error {
        TypeID type;

        explicit Error(TypeID type) : type(type) {}

        virtual void describe(io::OStream &out) const = 0;
        virtual view<Error*> unwrap() const {
            return {};
        }

        template <typename T>
        bool is() const requires std::derived_from<T, Error> {
            return errors::is(*this, type_id<T>);
        }

        template <typename T>
        const T *as() const requires std::derived_from<T, Error> {
            return errors::as(*this, type_id<T>);
        }

      private:
        virtual bool is(TypeID) const { return false; }

        friend Error const *errors::as(Error const &, TypeID);
        friend bool errors::is(Error const& err, TypeID target);
    } ;

    struct BasicError : Error {
        str msg;

        BasicError(str msg, TypeID type) : Error(type), msg(msg) {}
        BasicError(str msg) : Error(type_id<BasicError>), msg(msg) {}

        virtual void describe(io::OStream &out) const;
    } ;
    
    struct WriterToError : Error {
        io::WriterTo &writer_to;

        WriterToError(io::WriterTo &w) : Error(type_id<WriterToError>), writer_to(w) {}

        virtual void describe(io::OStream &out) const;
    } ;

    template <typename T, StringLiteral lit = StringLiteral<0>{}>
    struct ErrorBase : BasicError {     
        //static constexpr TypeID type = type_id<T>;

        ErrorBase() : BasicError(lit.value, type_id<T>) {};
    } ;

    template <typename T>
    struct ErrorBase<T, StringLiteral<0>{}> : Error {
        //static constexpr TypeID type = type_id<T>;

        ErrorBase() : Error(type_id<T>) {}
    } ;

    // //template <StringLiteral lit = StringLiteral<0>{}>
    // template <StringLiteral lit = StringLiteral<0>{}>
    // struct ErrorBase2 {        
    //     template <typename T>
    //     operator BasicError(this T const &) {
    //         return BasicError(lit.value);
    //     }

    //     BasicError berr = BasicError("x", type_id<int>);

    //     operator Error() {
    //         return berr;
    //     }

    // } ;

    // template<>
    // struct ErrorBase2<StringLiteral<0>{}> {
    //     template <typename T>
    //     struct Error : lib::Error {
    //         T t;

    //         Error(T&& t) : t(std::move(t)) {}

    //         virtual void describe(io::OStream &out) const override {
    //             t.describe(out);
    //         }
    //     } ;

    //     template <typename T>
    //     operator Error<T> (this T &&t) {
    //         return Error<T>(t);
    //     }
    // } ;

    struct HandlingErrorReporter;

    struct ErrorReporter {
        bool has_error = false;
        std::function<void(const Error&)> handler;

        ErrorReporter(std::invocable<const Error &> auto &&callable) : handler(callable) {}

        void report(const Error&);

        //ErrorReporter handle(std::function<bool(Error&)> const &f);
    } ;

    // struct HandlingErrorReporter : ErrorReporter {
    //     ErrorReporter           *upstream;
    //     std::function<bool(error const&)> handler;

    //     HandlingErrorReporter(std::function<bool(error const&)> const& f, ErrorReporter *upstream)
    //         : upstream(upstream), handler(f) {}

    //     virtual void report(error const& err) const override;
    // } ;

    struct error {
        ErrorReporter &reporter;

        error(ErrorReporter &&reporter) : reporter(reporter) {}
        error(ErrorReporter &reporter) : reporter(reporter) {}
        //error(std::function<void(const Error&)> const &f) : reporter(ErrorReporter(f)) {}

        void operator()(Error const &e) const {
            reporter.report(e);
        }

        void operator()(Error &&e) const {
            reporter.report(e);
        }

        template<typename ...Args>
        void operator()(str f, const Args &... args);
        
        explicit operator bool() const {
            return reporter.has_error;
        }

        static struct {
            operator error() { return ErrorReporter([](const Error &) {}); }
        } ignore;

        static struct {
            operator error() { return ErrorReporter([](const Error &e) { lib::panic(e); }); }
        } panic;
    } ;

    
    // struct Error2 {
    //     str msg;
        
    //     constexpr Error2(str message) : msg(message) {}
    // };

    // using Error = Error2;


    //extern const Error2 ErrUnspecified;

    // struct error2 {
    //     const Error2 *ref = nil;
        
    //     virtual void operator()(Error2 const &err) {
    //         this->ref = &err;
    //     }

    //     virtual void operator()(str) {
    //         this->ref = &ErrUnspecified;
    //     }

    //     template <typename... Args>
    //     void operator()(str fmt, const Args  & ...args);

    //     void operator()(error2 const& other) {
    //         if (other) {
    //             (*this)(*other.ref);
    //         } else {
    //             ref = nil;
    //         }
    //     }
        
    //     operator bool() const {
    //         return ref != nil;
    //     }
        
    //     struct Panic;
    //     struct Log;
    //     struct IgnoreAll;
    //     struct IgnoreOneImpl;
    //     struct IgnoreOne;

    //     static IgnoreAll ignore();
    //     IgnoreOne ignore(Error2 const& err);
        
    //     void fmt(fmt::Fmt&) const;
        
    //     bool handle(Error2 const& err);
        
    //     virtual ~error2() {}
        
    //     static Panic panic;
    //     static Log log;
    // } ;
    
    // struct error2::IgnoreAll {
    //     error2 err;
        
    //     operator struct error2& () { return err; }
    // } ;

    // struct error2::IgnoreOneImpl : error2 {
    //     error2 &parent;
    //     const Error2 *ignore;

    //     IgnoreOneImpl(error2 &parent, const Error2 *ignore) : parent(parent), ignore(ignore) {}

    //     virtual void operator()(Error2 const &err) override;
    //     virtual void operator()(str msg) override;
    // } ;

    // struct error2::IgnoreOne {
    //     IgnoreOneImpl impl;

    //     IgnoreOne(error2 &parent, const Error2 *ignore) : impl(parent, ignore) {}
    //     operator error2& () { return impl; }
    // } ;
    
    // struct error2::Panic : error2 {
    //     virtual void operator()(Error2 const &err) override;
    //     virtual void operator()(str msg) override;
    // };

    // struct error2::Log : error2 {
    //     virtual void operator()(Error2 const &err) override;
    //     virtual void operator()(str msg) override;
    // };
    
    // inline bool operator == (error2 err, Error2 const& derror) {
    //     return err.ref == &derror;
    // }
    
    // inline bool operator == (Error2 const& derror, error2 err) {
    //     return err.ref == &derror;
    // }
    
    // inline bool operator != (error2 err, Error2 const& derror) {
    //     return err.ref != &derror;
    // }
    
    // inline bool operator != (Error2 const& derror, error2 err) {
    //     return err.ref != &derror;
    // }
    
}


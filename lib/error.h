#pragma once

#include <type_traits>

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
        struct Writer;
    }

    
    struct Error;

    namespace errors {
        void log_error(const Error &);
    }

    struct Error;
    
    template <typename T>
    struct ErrorReporter;

    void panic(const Error &e);

    namespace errors {
        bool is(Error const& err, TypeID target);
        Error const *as(Error const &err, TypeID target);
        Error &cast(Error &err, TypeID target);
    }

    struct Error {
        TypeID type = nil;

        Error() {}
        explicit Error(TypeID type) : type(type) {}

        virtual void fmt(io::Writer &out, error err) const = 0;
        virtual view<Error*> unwrap() const {
            return {};
        }

        template <typename T>
        bool is() const requires std::derived_from<T, Error> {
            return errors::is(*this, type_id<T>);
        }

        bool same(Error const &other) const {
            return errors::is(*this, other.type);
        }

        template <typename T>
        const T *as() const requires std::derived_from<T, Error> {
            return static_cast<T const *>(errors::as(*this, type_id<T>));
        }

        template <typename T>
        T &cast() requires std::derived_from<T, Error> {
            return static_cast<T&>(errors::cast(*this, type_id<T>));
        }

        template <typename T>
        void init(this T &e) {
            if (e.type) {
                return;
            }
            if constexpr (std::is_same_v<T, Error>) {
                panic("can't init Error");
            }
            e.type = type_id<T>;
        }

        virtual ~Error() {}

      private:
        virtual bool is(TypeID type) const { return type == this->type; }

        friend Error const *errors::as(Error const &, TypeID);
        friend bool errors::is(Error const& err, TypeID target);
    } ;

    struct BasicError : Error {
        str msg;

        BasicError(str msg, TypeID type) : Error(type), msg(msg) {}
        BasicError(str msg) : Error(type_id<BasicError>), msg(msg) {}

        virtual void fmt(io::Writer &out, error err) const;
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

    // struct HandlingErrorReporter;

    struct ErrorReporterInterface {
        bool has_error = false;
        virtual void report(Error&) = 0;

        void report(Error &&e) {
            report((Error&) e);
        }

        template<typename ...Args>
        void report(str f, const Args &... args);
    } ;

    template <typename T>
    struct ErrorReporter : ErrorReporterInterface {
        
        //std::function<void(const Error&)> handler;

        //constexpr ErrorReporter(std::invocable<const Error &> auto &&callable) : handler(callable) {}

        T handler;

        ErrorReporter(T &&handler) : handler(handler) {};
        ErrorReporter(T &handler) : handler(handler) {};
        void report(Error &e) override {
            if (this->has_error) {
                return;
            }
        
            this->has_error = true;
            
            this->handler(e);
        }

        using ErrorReporterInterface::report;

        void operator()(Error &e) {
            this->report(e);
        }
        void operator()(Error &&e) {
            this->report(e);
        }

        template<typename ...Args>
        void operator()(str f, const Args &... args) {
            this->report(f, args...);
        }

        explicit operator bool() const {
            return this->has_error;
        }

    } ;

    template <typename T>
    struct ErrorReporterTmp : ErrorReporterInterface {
        const T *handler = nil;
        
        void report(Error &e) override {
            if (this->has_error) {
                return;
            }
        
            this->has_error = true;
            
            (*this->handler)(e);
        }
    };

    // struct HandlingErrorReporter : ErrorReporter {
    //     ErrorReporter           *upstream;
    //     std::function<bool(error const&)> handler;

    //     HandlingErrorReporter(std::function<bool(error const&)> const& f, ErrorReporter *upstream)
    //         : upstream(upstream), handler(f) {}

    //     virtual void report(error const& err) const override;
    // } ;

    struct error;
    struct IgnoringError;
    struct PanickingError;
    struct LoggingError;

    struct error {
        constexpr error(ErrorReporterInterface &&reporter) : reporter(reporter) {}
        constexpr error(ErrorReporterInterface &reporter) : reporter(reporter) {}
        
        template <typename T>
        constexpr error(T const &fn, ErrorReporterTmp<T> &&tmp = {}) 
        requires (std::is_invocable_v<T, Error&> && !std::is_base_of_v<error, T> && !std::is_base_of_v<ErrorReporterInterface, T>)
        // requires (std::is_invocable_v<T, Error&> && !std::is_base_of_v<error, T>())
        : reporter(tmp) {
            tmp.handler = &fn;
        }

        error(error const& other) : reporter(other.reporter) {}
        error(error &other) : reporter(other.reporter) {}
        error(error &&other) : reporter(other.reporter) {}

        //error(std::function<void(const Error&)> const &f) : reporter(ErrorReporter(f)) {}

        void operator()(str s) const {
            reporter.report(s);
        }

        template <typename T>
        void operator()(T &&e) const requires std::is_base_of_v<Error, std::remove_cvref_t<T>> {
            e.init();
            reporter.report(e);
        }
        // void operator()(Error &&e) const {
        //     reporter.report(e);
        // }

        // void operator()(Error &&e) const {
        //     reporter.report(e);
        // }

        template<typename Arg, typename ...Args>
        void operator()(str f, Arg const & arg, const Args &... args) {
            reporter.report(f, arg, args...);
        }
        
        explicit operator bool() const {
            return reporter.has_error;
        }

        inline static struct {
            operator IgnoringError();
        } ignore;

        inline static struct {
            operator PanickingError(); // { return ErrorReporter([](const Error &e) { lib::panic(e); }); }
        } panic;

        inline static struct {
            operator LoggingError(); // { return ErrorReporter([](const Error &e) { errors::log_error(e); }); }
        } log;

    private:
        ErrorReporterInterface &reporter;
    } ;

    struct IgnoringError : error {
        ErrorReporter<void(*)(const Error&)> error_reporter;

        IgnoringError();
    };

    struct PanickingError : error {
        ErrorReporter<void(*)(const Error&)> error_reporter;

        PanickingError();
    };

    struct LoggingError : error {
        ErrorReporter<void(*)(const Error&)> error_reporter;

        LoggingError();
    };

    struct SavedError : Error {
        String msg;

        SavedError(TypeID id, str msg);
        virtual void fmt(io::Writer &out, error err) const override;
    } ;

    struct ErrorRecorder : ErrorReporterInterface {
        String msg;
        TypeID type = nil;

        void report(Error&) override;
        void report(Error const &);
        using ErrorReporterInterface::report;
        void fmt(io::Writer &out, error err) const;
        explicit operator bool() const {
            return this->has_error;
        }

        bool is(Error const &other);

        SavedError to_error() const;

        bool operator==(ErrorRecorder const &other) const;
    };
}


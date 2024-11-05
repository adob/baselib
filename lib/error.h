#pragma once

#include "types.h"
#include "str.h"

namespace lib {
    
    namespace fmt {
        struct Fmt;
    }
    
    struct deferror {
        str msg;
        
        constexpr deferror(str message) : msg(message) {}
    };

    extern const deferror ErrUnspecified;

    struct error {
        const deferror *ref = nil;
        
        virtual void operator()(deferror const &err) {
            this->ref = &err;
        }

        virtual void operator()(str) {
            this->ref = &ErrUnspecified;
        }

        template <typename... Args>
        void operator()(str fmt, const Args  & ...args);

        void operator()(error const& other) {
            if (other) {
                (*this)(*other.ref);
            } else {
                ref = nil;
            }
        }
        
        operator bool() const {
            return ref != nil;
        }
        
        struct Panic;
        struct Log;
        struct IgnoreAll;
        struct IgnoreOneImpl;
        struct IgnoreOne;

        static IgnoreAll ignore();
        IgnoreOne ignore(deferror const& err);
        
        void fmt(fmt::Fmt&) const;
        
        bool handle(deferror const& err);
        
        virtual ~error() {}
        
        static Panic panic;
        static Log log;
    } ;
    
    struct error::IgnoreAll {
        error err;
        
        operator error& () { return err; }
    } ;

    struct error::IgnoreOneImpl : error {
        error &parent;
        const deferror *ignore;

        IgnoreOneImpl(error &parent, const deferror *ignore) : parent(parent), ignore(ignore) {}

        virtual void operator()(deferror const &err) override;
        virtual void operator()(str msg) override;
    } ;

    struct error::IgnoreOne {
        IgnoreOneImpl impl;

        IgnoreOne(error &parent, const deferror *ignore) : impl(parent, ignore) {}
        operator error& () { return impl; }
    } ;
    
    struct error::Panic : error {
        virtual void operator()(deferror const &err) override;
        virtual void operator()(str msg) override;
    };

    struct error::Log : error {
        virtual void operator()(deferror const &err) override;
        virtual void operator()(str msg) override;
    };
    
    inline bool operator == (error err, deferror const& derror) {
        return err.ref == &derror;
    }
    
    inline bool operator == (deferror const& derror, error err) {
        return err.ref == &derror;
    }
    
    inline bool operator != (error err, deferror const& derror) {
        return err.ref != &derror;
    }
    
    inline bool operator != (deferror const& derror, error err) {
        return err.ref != &derror;
    }
    
}

#pragma once

#include "lib/sync/mutex.h"
#include "lib/testing/testing.h"
#include <unordered_map>

namespace lib::testing::detail {
    struct FilterMatch {
        struct Result {
            bool ok;
            bool partial;
        } ;
        // matches checks the name against the receiver's pattern strings using the
        // given match function.
        virtual Result matches(view<str> name, std::function<bool(str,str,error)> match_string) = 0;

        // verify checks that the receiver's pattern strings are valid filters by
        // calling the given match function.
        virtual void verify(str name, std::function<bool(str,str,error)> match_string, error) = 0;

        virtual ~FilterMatch() = default;
    } ;

    struct Matcher {
        std::unique_ptr<FilterMatch> filter = nil;
        std::unique_ptr<FilterMatch> skip = nil;

        std::function<bool(str,str,error)> match_func;

        sync::Mutex mu;

        // sub_names is used to deduplicate subtest names.
        // Each key is the subtest name joined to the deduplicated name of the parent test.
        // Each value is the count of the number of occurrences of the given subtest name
        // already seen.
        std::unordered_map<String, int32> sub_names;

        std::tuple<String, bool, bool> full_name(Common const *c, str subname);
        
        // unique creates a unique name for the given parent and subname by affixing it
        // with one or more counts, if necessary.
        String unique(str parent, str subname);
    } ;

    Matcher new_matcher(std::function<bool(str pat, str s, error)> const &match_string,
                str patterns, str name, str skips);
} ;
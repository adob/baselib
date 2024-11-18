#include "testing.h"

using namespace lib;
using namespace testing;
        
void T::fail() {
    failed = true;
}

// void T::operator () (bool b) {
//     if (!b) {
//         errors.push_back({"Failed", debug::backtrace(2) });
//         fail();
//     }
// }

module;
#include "isprint_impl.h"

export module lib.strconv.isprint;
import lib.array;
import lib.types;


export extern "C++"
namespace lib::strconv {    
    extern const arr<uint16> IsPrint16;
    extern const arr<uint16> IsNotPrint16;
    extern const arr<uint32> IsPrint32;
    extern const arr<uint16> IsNotPrint32;
    extern const arr<uint16> IsGraphic;
}

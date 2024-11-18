#pragma once

#include "../base.h"
#include "lib/os/file.h"

namespace lib::os {
    // syscall_mode returns the syscall-specific mode bits from portable mode bits.
    uint32 syscall_mode(FileMode i);
}
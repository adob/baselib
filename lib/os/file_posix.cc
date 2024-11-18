#include "file_posix.h"

#include <sys/stat.h>

using namespace lib;
using namespace os;

 uint32 os::syscall_mode(FileMode i) {
    uint32 o = uint32(i.perm());

    if ((i&ModeSetuid) != 0) {
		o |= S_ISUID;
	}
	if ((i&ModeSetgid) != 0) {
		o |= S_ISGID;
	}
	if ((i&ModeSticky) != 0) {
		o |= S_ISVTX;
	}

    return o;
 }
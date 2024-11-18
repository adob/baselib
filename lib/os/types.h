#pragma once

#include <fcntl.h>

#include "lib/fs/fs.h"


namespace lib::os {
    using namespace lib;

    using fs::FileMode;
    
    struct FileInfo : fs::FileInfo {
        struct ::stat stat;
    };

    // The single letters are the abbreviations
	// used by the String method's formatting.
    const FileMode 
        ModeDir        = fs::ModeDir,        // d: is a directory
        ModeAppend     = fs::ModeAppend,     // a: append-only
        ModeExclusive  = fs::ModeExclusive,  // l: exclusive use
        ModeTemporary  = fs::ModeTemporary,  // T: temporary file; Plan 9 only
        ModeSymlink    = fs::ModeSymlink,    // L: symbolic link
        ModeDevice     = fs::ModeDevice,     // D: device file
        ModeNamedPipe  = fs::ModeNamedPipe,  // p: named pipe (FIFO)
        ModeSocket     = fs::ModeSocket,     // S: Unix domain socket
        ModeSetuid     = fs::ModeSetuid,     // u: setuid
        ModeSetgid     = fs::ModeSetgid,     // g: setgid
        ModeCharDevice = fs::ModeCharDevice, // c: Unix character device, when ModeDevice is set
        ModeSticky     = fs::ModeSticky,     // t: sticky
        ModeIrregular = fs::ModeIrregular,   // ?: non-regular file; nothing else is known about this file

        // Mask for the type bits. For regular files, none will be set.
        ModeType = fs::ModeType,

        ModePerm = fs::ModePerm; // Unix permission bits, 0o777
}
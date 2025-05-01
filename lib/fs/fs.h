#pragma once

#include "../base.h"
#include "lib/io/io.h"
#include "lib/time/time.h"


namespace lib::fs {

    // A FileMode represents a file's mode and permission bits.
    // The bits have the same definition on all systems, so that
    // information about files can be moved from one system
    // to another portably. Not all bits apply to all systems.
    // The only required bit is [ModeDir] for directories.
    struct FileMode : bitflag<uint32> {
        using bitflag::bitflag;

        // Perm returns the Unix permission bits in m (m & [ModePerm]).
        FileMode perm(this FileMode m);
    } const
        ModeDir        = uint32(1) << (32 - 1),  // d: is a directory
        ModeAppend     = uint32(1) << (32 - 2),  // a: append-only
        ModeExclusive  = uint32(1) << (32 - 3),  // l: exclusive use
        ModeTemporary  = uint32(1) << (32 - 4),  // T: temporary file; Plan 9 only
        ModeSymlink    = uint32(1) << (32 - 5),  // L: symbolic link
        ModeDevice     = uint32(1) << (32 - 6),  // D: device file
        ModeNamedPipe  = uint32(1) << (32 - 7),  // p: named pipe (FIFO)
        ModeSocket     = uint32(1) << (32 - 8),  // S: Unix domain socket
        ModeSetuid     = uint32(1) << (32 - 9),  // u: setuid
        ModeSetgid     = uint32(1) << (32 - 10), // g: setgid
        ModeCharDevice = uint32(1) << (32 - 11), // c: Unix character device, when ModeDevice is set
        ModeSticky     = uint32(1) << (32 - 12), // t: sticky
        ModeIrregular  = uint32(1) << (32 - 13), // ?: non-regular file; nothing else is known about this file

        // Mask for the type bits. For regular files, none will be set.
        ModeType = ModeDir | ModeSymlink | ModeNamedPipe | ModeSocket | ModeDevice | ModeCharDevice | ModeIrregular,

        ModePerm = 0777; // Unix permission bits

    struct ErrInvalid    : ErrorBase<ErrInvalid, "invalid argument"> {};
    struct ErrPermission : ErrorBase<ErrPermission, "permission denied"> {};
    struct ErrExist      : ErrorBase<ErrExist, "file already exists"> {};
    struct ErrNotExist   : ErrorBase<ErrNotExist, "file does not exist"> {};
    struct ErrClosed     : ErrorBase<ErrClosed, "file already closed"> {};

    struct PathError : ErrorBase<PathError> {
        str op;
        str path;
        const Error *err = nil;

        PathError(str op, str path, Error const &err) : op(op), path(path), err(&err) {}

        virtual void fmt(io::Writer &out, error err) const override;
    } ;

    struct FileInfo {
        String     name;
        int64      size {};
        FileMode   mode;
        time::time mod_time;
        bool       is_dir {};
    } ;
}
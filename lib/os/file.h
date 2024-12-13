#pragma once

#include "lib/base.h"
#include "lib/io.h"
#include "../fs/fs.h"

#include "stat.h"
#include "error.h"

namespace lib::os {

    struct File : io::Buffered {
        int fd = -1;
        CString name;
        //int efd = -1;
        
        File() = default;
        explicit File(int fd) : fd(fd) {}
        File(File const&) = delete;
        File(File&& other);

        FileInfo stat(error err);
        
        // read reads up to len(b) bytes into b and returns the number of bytes read. At end of 
        // file, 0 is returned.
        io::ReadResult direct_read(buf b, error err) override;

        size direct_write(str data, error err) override;
        
        File& operator= (File const&) = delete;
        File& operator= (File&& other);
        
        // Close closes the [File], rendering it unusable for I/O.
        // On files that support [File.SetDeadline], any pending I/O operations will
        // be canceled and return immediately with an [ErrClosed] error.
        // Close will return an error if it has already been called.
        void close(error err) override;
        
        ~File();
        
        friend File open(str, int, error);

      private:
        void wrap_err(str op, const Error &wrapped, error err);
    };
    
    // Open opens the named file for reading. If successful, methods on
    // the returned file can be used for reading; the associated file
    // descriptor has mode O_RDONLY.
    // If there is an error, it will be of type *PathError.
    File open(str name, error err);

    // OpenFile is the generalized open call; most users will use Open
    // or Create instead. It opens the named file with specified flag
    // (O_RDONLY etc.). If the file does not exist, and the O_CREATE flag
    // is passed, it is created with mode perm (before umask). If successful,
    // methods on the returned File can be used for I/O.
    // If there is an error, it will be of type *PathError.
    File open_file(str name, int flag, FileMode perm, error err);

    // WriteFile writes data to the named file, creating it if necessary.
    // If the file does not exist, WriteFile creates it with permissions perm (before umask);
    // otherwise WriteFile truncates it before writing, without changing permissions.
    // Since WriteFile requires multiple system calls to complete, a failure mid-operation
    // can leave the file in a partially written state.
    //
    // perm defaults to 0666
    void write_file(str name, str data, FileMode perm, error err);
    void write_file(str name, str data, error err);

    String read_file(str name, error err);

}

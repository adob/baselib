#pragma once

#include "lib/base.h"
#include "lib/io.h"

#include "stat.h"

namespace lib::os {

    struct File : io::Buffered {
        int fd = -1;
        //int efd = -1;
        
        File() = default;
        explicit File(int fd) : fd(fd) {}
        File(File const&) = delete;
        File(File&& other);

        FileInfo stat(error &err);
        
        // read reads up to len(b) bytes into b and returns the number of bytes read. At end of 
        // file, 0 is returned.
        size direct_read(buf b, error& err) override;

        size direct_write(str data, error& err) override;
        
        File& operator= (File const&) = delete;
        File& operator= (File&& other);
        
        void close(error &err) override;
        
        ~File();
        
        friend File open(str, int, error&);
    };
    
    File open(str name, int flags, error &err);

    size write_file(str name, str data, int perm, error &err);
    size write_file(str name, str data, error &err);
    String read_file(str name, error &err);
}

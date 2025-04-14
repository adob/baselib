// StdStream

#include "stdio.h"
#include "lib/io/utils.h"
#include <unistd.h>

using namespace lib;
using namespace os;

StdStream os::stdout(::stdout);
StdStream os::stderr(::stderr);

io::ReadResult os::StdStream::direct_read(buf bytes, error err) {
    size n = size(::fread(bytes.data, 1, usize(len(bytes)), file));
 
     if (n != len(bytes)) {
         if (::ferror(file)) {
             err(io::ErrIO());
             return {n, false};
         }
 
         if (::feof(file)) {
             return {n, true};
         }
    }
 
    return {n, false};
 }
 
 size os::StdStream::direct_write(str data, error err) {
    // ::write(1, "<", 1);
    size n = ::write(1, data.data, data.len);
    // ::write(1, ">", 1);
    // printf("%ld:", len(data));
    // size n = size(::fwrite(data.data, 1, usize(len(data)), file));
 
     if (n != len(data)) {
        panic("write error");
        if (::ferror(file)) {
            err(io::ErrIO());
            return n;
        }

        if (::feof(file)) {
            err(io::ErrUnexpectedEOF());
            return n;
        }

    err(io::ErrIO());
    return n;
    }
 
    return n;
 }
#ifdef ARDUINO
#include <Arduino.h>

import lib.str;
import lib.error;
import lib.types;
import lib.io;
import <stdio.h>;

using namespace lib;
using namespace io;

StdStream io::in(stdin, 0);
StdStream io::out(stdout, 1);
StdStream io::err(stderr, 2);

io::ReadResult io::StdStream::direct_read(buf bytes, error err) {
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
 
// Both output streams use Arduino's default Serial device; the application initializes it.
size io::StdStream::direct_write(str data, error err) {
    size n = size(Serial.write((const uint8_t *)data.data, usize(len(data))));
    if (n != len(data)) {
        err(io::ErrIO());
    }
    return n;
}

#endif

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h> 
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "file.h"
#include "error.h"

using namespace lib;
using namespace os;

os::File::File(File&& other)
    : fd(other.fd)
    , io::Buffered(std::move(other))
     { 
        other.fd = -1; 
}

void os::File::close(error &err) {
    if (fd == -1) {
        return;
    }    
    int ret = ::close(fd);
    fd = -1;
    if (ret) {
         err(os::from_errno(errno));
         return;
    }
}

os::File os::open(str name, int flags, error &err) {
    int fd = ::open(name.c_str(), flags);
    if (fd == -1) {
        err(os::from_errno(errno));
    }
    File f(fd);
    
    if (flags & (O_RDONLY | O_RDWR)) {
        f.resize_readbuf(4096);
    }
    
    if (flags & (O_WRONLY | O_RDWR)) {
        f.resize_writebuf(4096);
    }
    
//     efd = ::eventfd(0, 0);
//     if (efd == -1) {
//         panic(os::from_errno(errno));
//     }
    
    return f;
}

FileInfo File::stat(error &err) {
    struct stat statbuf;

    int r = ::fstat(fd, &statbuf);
    if (r == -1) {
        err(from_errno(errno));
        return {};
    }

    return {
        .size = statbuf.st_size,
    };
}

size os::write_file(str name, str data, error &err) {
    return write_file(name, data, 0666, err);
}

size os::write_file(str name, str data, int perm, error &err) {
    int fd = ::open(name.c_str(), O_WRONLY|O_CREAT|O_TRUNC, perm);
    if (fd == -1) {
        err(os::from_errno(errno));
    }

    File f(fd);
    size total = f.direct_write(data, err);
    if (err) {
        return total;
    }

    f.close(err);
    return total;
}

String os::read_file(str path, error &err) {
    os::File f = open(path, O_RDONLY, err);
    
    size sz = 0;
    error staterr;

    FileInfo info = f.stat(staterr);
    if (!staterr) {
        sz = info.size;
    }

    if (sz < 512) {
        sz = 512;
    }

    String s(sz);
    
    for (;;) {
        size n = f.direct_read(s.buffer[len(s), len(s.buffer)], err);
        s.length += n;
        if (err) {
            if (err == io::EOF) {
                err = {};
            }
            return s;
        }

        if (len(s) >= len(s.buffer)) {
            s.buffer.resize(len(s.buffer)*2);
        }
    } 

}

size os::File::direct_read(buf b, error& err) {
  retry:
    size ret = ::read(fd, b.data, b.len);
    if (ret == -1) {
        if (errno == EINTR) {
            goto retry;
        }
        err(os::from_errno(errno));
        return 0;
    }
    if (ret == 0) {
        err(io::EOF);
    }
    
    //print "direct_read <%d> <%x>" % len(b), b[0];
    return ret;
}

// size os::File::read_full(buf b, error& err) {
//     size total = 0;
//     size want = len(buf);
//     
//   again:
//     size ret = this->read(b, err);
//     if (err) {
//         return ret;
//     }
//     
//     if (ret == 0) {
//         err(ErrEOF);
//         return total;
//     }
//     
//     total += ret;
//     if (total < want) {
//         b = b(total);
//         goto again;
//     }
//     
//     return total;
// }

size os::File::direct_write(str data, error& err) {
    // printf("DIRECT WRITE %d\n", int(len(data)));
    size total = 0;
    size want = len(data);

    retry:
    size ret = ::write(fd, data.data, data.len);
    if (ret == -1) {
        if (errno == EINTR) {
            goto retry;
        }
        err(os::from_errno(errno));
        return total;
    }
    
    total += ret;
    if (total < want) {
        if (ret == 0) {
            err(io::ErrUnexpectedEOF);
            return total;
        }
        data = data(total);
        goto retry;
    }
    return total;
}

File& File::operator = (File&& other) {
    if (this == &other) {
        return *this;
    }

    fd = other.fd;
    other.fd = -1;

    io::Buffered::operator=(std::move(other));

    return *this;
}

os::File::~File() {
    if (fd != -1) {
        flush(error::ignore());
        ::close(fd);  // ignore error
    }
}

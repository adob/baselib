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
#include "lib/error.h"
#include "lib/fmt/fmt.h"
#include "lib/fs/fs.h"
#include "lib/filepath/path.h"
#include "lib/io/io_stream.h"
#include "lib/io/utils.h"
#include "lib/os/file_posix.h"
#include "lib/os/types.h"

#include "lib/print.h"
#include "lib/str.h"


using namespace lib;
using namespace os;


os::File::File(File&& other)
    : io::Buffered(std::move(other)),
      fd(other.fd) { 
    other.fd = -1; 
}


void os::File::close(error err) {
    int fd = this->fd;

    if (fd == -1) {
        return err(PathError("close", this->name, ErrClosed()));
    } 

    flush(err);

    // invalidate the file descriptor early
    // this is because Linux invalidates the file descriptor early even if there
    // is a failure to close (such as an I/O error)
    this->fd = -1;

retry:
    int ret = ::close(fd);
    if (ret == -1) {
        if (errno == EINTR) {
            goto retry;
        }

        return err(PathError("close", this->name, Errno(errno)));
    }
}

os::File os::open(str name, error err) {
    return open_file(name, O_RDONLY, 0, err);
}

os::File os::open_file(str name, int flag, FileMode perm, error err) {
    File f;

    f.name = name;

retry:
    f.fd = ::open(f.name, flag|O_CLOEXEC, syscall_mode(perm));
    if (f.fd == -1) {
        if (errno == EINTR) {
            goto retry;
        }

        err(PathError("open", name, Errno(errno)));
        return f;
    }

    if (flag & (O_RDONLY | O_RDWR)) {
        f.resize_readbuf(4096);
    }
    
    if (flag & (O_WRONLY | O_RDWR)) {
        f.resize_writebuf(4096);
    }

    //f.flags = flags;
    return f;
}

static void fill_file_info_from_sys(FileInfo *fi, str name) {
    fi->name = filepath::base(name);
    fi->size = fi->stat.st_size;
    fi->mod_time = time::unix(fi->stat.st_mtim);
    fi->mode = FileMode(fi->stat.st_mode & 0777);

    switch (fi->stat.st_mode & S_IFMT) {
        case S_IFBLK: fi->mode |= ModeDevice; break;
        case S_IFCHR: fi->mode |= ModeDevice | ModeCharDevice; break;
        case S_IFDIR: fi->mode |= ModeDir; break;
        case S_IFIFO: fi->mode |= ModeNamedPipe; break;
        case S_IFLNK: fi->mode |= ModeSymlink; break;
        case S_IFREG: /* do nothing */; break;
        case S_IFSOCK: fi->mode |= ModeSocket; break;
    }

    if (fi->stat.st_mode & S_ISGID) {
        fi->mode |= ModeSetgid;
    }

    if (fi->stat.st_mode & S_ISUID) {
        fi->mode |= ModeSetuid;
    }

    if (fi->stat.st_mode & S_ISVTX ) {
        fi->mode |= ModeSticky;
    }
}

FileInfo File::stat(error err) {
    FileInfo fi;

retry:
    int r = ::fstat(fd, &fi.stat);
    if (r == -1) {
        if (errno == EINTR) {
            goto retry;
        }
        
        err(PathError("stat", name, Errno(errno)));
        return fi;
    }

    fill_file_info_from_sys(&fi, name);

    return fi;
}

void os::write_file(str name, str data, error err) {
    write_file(name, data, 0666, err);
}

void os::write_file(str name, str data, FileMode perm, error err) {
    File f = open_file(name, O_WRONLY|O_CREAT|O_TRUNC, perm, err);
    if (err) {
        return;
    }

    f.direct_write(data, err);
    if (err) {
        return;
    }

    f.close(err);
}

String os::read_file(str path, error err) {
    os::File f = open(path, err);
    if (err) {
        return "";
    }

    
    FileInfo info = f.stat(error::ignore);
    size file_size = size(info.size);
    if (file_size != info.size) {
        file_size = 0;
    }
    //file_size++; // one byte for final read at EOF

    // If a file claims a small size, read at least 512 bytes.
	// In particular, files in Linux's /proc claim size 0 but
	// then do not work right if read in small pieces,
	// so an initial read of 1 byte would not work correctly.
	if (file_size < 512) {
		file_size = 512;
	}

    String s;
    s.ensure(file_size);
    size bytes_read = 0;
    
    for (;;) {
        io::ReadResult r = f.direct_read(s.buffer+bytes_read, err);
        bytes_read += r.nbytes;

        if (err || r.eof) {
            break;
        }

        s.ensure(bytes_read+1);
    }

    s.expand(bytes_read);
    return s;

        
    // }
    // size size = info.size;

    // if (!staterr) {
    //     sz = info.size;
    // }

    // if (sz < 512) {
    //     sz = 512;
    // }

    // String s(sz);
    
    // for (;;) {
    //     size n = f.direct_read(s.buffer[len(s), len(s.buffer)], err);
    //     s.length += n;
    //     if (err) {
    //         if (err == io::EOF) {
    //             err = {};
    //         }
    //         return s;
    //     }

    //     if (len(s) >= len(s.buffer)) {
    //         s.buffer.resize(len(s.buffer)*2);
    //     }
    // } 

}

io::ReadResult os::File::direct_read(buf b, error err) {
    //y3printf("DIRECT READ\n");

    io::ReadResult r;
  retry:
    size ret = ::read(fd, b.data, b.len);
    if (ret == -1) {
        if (errno == EINTR) {
            goto retry;
        }
        err(PathError("read", this->name, Errno(errno)));
        return r;
    }

    if (ret == 0) {
        r.eof = true;
    }
    
    r.nbytes += ret;


    // print "direct_read <%d> <%x>" % ret, b[0];
    // for (size i = 0; i < ret; i++) {
    //     printf("%02x ", b[i]);
    // }
    // printf("\n");
    return r;
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

size os::File::direct_write(str data, error err) {
    //printf("DIRECT WRITE %d\n", int(len(data)));
    size total = 0;
    size want = len(data);

    retry:
    size n = ::write(fd, data.data, data.len);
    if (n == -1) {
        if (errno == EINTR) {
            goto retry;
        }
        wrap_err("write", Errno(errno), err);

        return total;
    }

    if (n > len(data)) {
        panic(fmt::sprintf("invalid return from write: got %d from write of %d", n, len(data)));
    }
    
    total += n;
    if (total < want) {
        if (n == 0) {
            wrap_err("write", io::ErrUnexpectedEOF(), err);
            return total;
        }
        data = data+total;
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
    //printf("os::File destructor %d\n", fd);

    if (fd == -1) {
        return;
    }
    
    this->close(error::ignore);
}

void File::wrap_err(str op, const lib::Error &wrapped, error err) {
    err(PathError(op, this->name, wrapped));
}


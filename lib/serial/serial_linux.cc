#include "serial_linux.h"
#include "lib/error.h"
#include "lib/os/error.h"
#include "lib/print.h"

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <linux/serial.h>
#include <sys/ioctl.h>

using namespace lib;
using namespace serial;

Port serial::open(str path, error err) {
    Port port;

    os::File f = os::open_file(path, O_RDWR | O_NOCTTY | O_NONBLOCK, 0, err);
    *((os::File*) &port) = std::move(f);
    if (err) {
        return port;
    }

    struct termios tty;

    if (::tcgetattr(port.fd, &tty) != 0) {
        err(os::SyscallError("tcgetattr", errno));
        return port;
    }

    // Set Baud Rate
    ::cfsetispeed(&tty, B921600);
    ::cfsetospeed(&tty, B921600);

    // 8-bit characters
    tty.c_cflag &= ~CSIZE; 
    tty.c_cflag |= CS8;

    // No parity bit
    tty.c_cflag &= ~PARENB;

    // One stop bit
    tty.c_cflag &= ~CSTOPB;

    // Disable hardware flow control
    tty.c_cflag &= ~CRTSCTS;

    // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_cflag |= CREAD | CLOCAL;

    // Disable canonical mode, echo, extended input processing, and signal characters
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // Disable output processing
    tty.c_oflag &= ~OPOST;

    // Set in/out baud rate
    ::cfsetspeed(&tty, B921600);

    // Set port to raw mode
    ::cfmakeraw(&tty);
    tty.c_cc[VMIN] = 1;    // Minimum number of characters for noncanonical read (MIN).
    tty.c_cc[VTIME] = 0;  // Timeout in deciseconds for noncanonical read (TIME).
    

    // Apply the settings to the port
    if (::tcsetattr(port.fd, TCSANOW, &tty) != 0) {
        err(os::PathError("tcsetattr", path, os::Errno(errno)));
        return port;
    }

    // set low latency
    struct serial_struct serinfo;
    if (::ioctl(port.fd, TIOCGSERIAL, &serinfo) < 0) {
        err(os::PathError("ioctl(TIOCGSERIAL)", path, os::Errno(errno)));
        return port;
    }

    serinfo.flags |= ASYNC_LOW_LATENCY;

    if (::ioctl(port.fd, TIOCSSERIAL, &serinfo) < 0) {
        err(os::PathError("ioctl(TIOCSSERIAL)", path, os::Errno(errno)));
        return port;
    }

    // attempt to discard buffered data
    if (tcflush(port.fd, TCOFLUSH) < 0) {
        err(os::PathError("tcflush(TCOFLUSH)", path, os::Errno(errno)));
        return port;
    }

    // discard data
    byte buf[512];
    for (;;) {
        size n = read(port.fd, buf, sizeof buf);
        if (n <= 0) {
            break;  
        }
        print "serial_linux: discared %d bytes" % n;
    }

    int flags = fcntl(port.fd, F_GETFL, 0);
    if (flags == -1) {
        err(os::PathError("fcntl(F_GETFL)", path, os::Errno(errno)));
        return port;
    }
    flags &= ~O_NONBLOCK;
    if (fcntl(port.fd, F_SETFL, flags) < 0) {
        err(os::PathError("fcntl(F_SETFL)", path, os::Errno(errno)));
        return port;
    }
    

    return port;
}
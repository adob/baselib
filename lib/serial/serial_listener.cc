export module lib.serial.serial_listener;
export import lib.error;
export import lib.io;
export import lib.sync.mutex;


export extern "C++" {
namespace lib::serial {
    // Conn represents one logical session on a serial port. A peer establishes
    // a session by asserting the port's connection signals (such as DTR and
    // RTS). Closing the connection ends only that session, not the underlying
    // port; input is ignored until the peer disconnects.
    struct Conn : io::StaticBuffered<512, 512> {
        sync::Mutex write_mtx;
    } ;

    // Listener accepts one serial session at a time. After a connection is
    // closed, accept waits for the peer to disconnect and establish a new
    // session before returning the connection again. Serial sessions have no
    // local or remote addresses.
    struct Listener {
        virtual Conn& accept(error) = 0;
        virtual ~Listener() {}
    } ;
}

}

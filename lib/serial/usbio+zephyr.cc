#include "lib/serial/usbio+zephyr.h"

#include "lib/os/error.h"

namespace lib::serial {

io::ReadResult USBConn::direct_read(buf buffer, error err)
{
	size_t bytes_read;
	int ret = usbd_cdc_acm_read(&conn_, buffer.data, len(buffer), &bytes_read);

	if (ret != 0) {
		err(os::Errno(-ret));
	}

	return {.nbytes = size(bytes_read), .eof = ret == 0 && bytes_read == 0};
}

size USBConn::direct_write(str buffer, error err)
{
	size_t bytes_written;
	int ret = usbd_cdc_acm_write(&conn_, (const byte *)buffer.data, len(buffer),
				     &bytes_written);

	if (ret != 0) {
		err(os::Errno(-ret));
	}

	return size(bytes_written);
}

Conn &USBListener::accept(error err)
{
	if (conn_.conn_.dev != nullptr) {
		(void)usbd_cdc_acm_close(&conn_.conn_);
	}

	conn_.reset();
	int ret = usbd_cdc_acm_accept(dev_, &conn_.conn_);
	if (ret != 0) {
		err(os::Errno(-ret));
	}

	return conn_;
}

} // namespace lib::serial

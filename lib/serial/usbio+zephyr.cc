module;
#include "usbio_impl+zephyr.h"

export module lib.serial.usbio;
import <zephyr/device.h>;
import <zephyr/usb/class/usbd_cdc_acm.h>;

import :serial_listener;
import lib.types;


export extern "C++"
namespace lib::serial {

class USBConn final : public Conn {
public:
	io::ReadResult direct_read(buf buffer, error err) override;
	size direct_write(str buffer, error err) override;

private:
	struct usbd_cdc_acm_conn conn_ = {};

	friend class USBListener;
};

class USBListener final : public Listener, nonmovable {
public:
	explicit USBListener(const struct device *dev) : dev_(dev) {}

	Conn &accept(error err) override;

private:
	const struct device *dev_;
	USBConn conn_;
};

} // namespace lib::serial

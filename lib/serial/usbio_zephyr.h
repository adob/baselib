#pragma once

#include <zephyr/device.h>
#include <zephyr/usb/class/usbd_cdc_acm.h>

#include "lib/serial/serial_listener.h"
#include "lib/types.h"

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

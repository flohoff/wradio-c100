#ifndef USB_HPP
#define USB_HPP

#include <vector>
#include <cstdint>

#include "libusb-1.0/libusb.h"

class USB {
	private:
		struct libusb_device	**devices;
		int			numdevices;
	public:

	class Device;

	USB() {
		libusb_init(NULL);
		//libusb_set_debug(NULL, 255);

		numdevices=libusb_get_device_list(NULL, &devices);
	}

	~USB() {
		libusb_free_device_list(devices, 1);
	}

	std::shared_ptr<USB::Device> device_by_vendprod(uint16_t vendor, uint16_t product) {
		for (int i=0;i<numdevices;i++) {
			struct libusb_device_descriptor desc;

			libusb_device *dev = devices[i];
			libusb_get_device_descriptor(dev, &desc);

			if (desc.idVendor == vendor
					&& desc.idProduct == product) {

				return std::make_shared<USB::Device>(dev);
			}
		}

		return nullptr;
	};
};

class USB::Device {
	private:
		libusb_device_handle		*handle;
	public:

	Device(struct libusb_device *dev) {
		int r=libusb_open(dev, &handle);

		if (r)
			throw;
	}

	~Device() {
		libusb_close(handle);
	}

	int bulk(int endpoint, std::vector<uint8_t> &buffer, unsigned int timeout) {
		int transferred;
		int r=libusb_bulk_transfer(handle, endpoint, buffer.data(), buffer.size(), &transferred, timeout*10);
		if (!r)
			return r;
		return transferred;
	}

	int bulk_write(int endpoint, std::vector<uint8_t> &buffer, unsigned int timeout) {
		int transferred;
		int r=libusb_bulk_transfer(handle, endpoint, buffer.data(), buffer.size(), &transferred, timeout*10);
		if (r)
			return r;
		return transferred;
	}

	int bulk_read(int endpoint, std::vector<uint8_t> &buffer, unsigned int timeout) {
		int transferred;
		int r=libusb_bulk_transfer(handle, endpoint, buffer.data(), buffer.size(), &transferred, timeout*10);
		if (r)
			return r;
		buffer.resize(transferred);
		return transferred;
	}
};

#endif

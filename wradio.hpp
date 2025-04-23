#ifndef WRADIO_HPP
#define WRADIO_HPP

#include <vector>
#include <memory>
#include <iostream>
#include <iomanip>
#include <cstdint>

#include "usb.h"

class WRadio {
	private:
		std::shared_ptr<USB::Device>	dev;

		enum REGISTER_PAGE {
			REGISTER_PAGE_OFDM = 0x02,  //For 1seg
			REGISTER_PAGE_FEC =  0x03,  //For 1seg
			REGISTER_PAGE_COMM = 0x04,
			REGISTER_PAGE_FM =   0x06,  //T-DMB OFDM/FM
			REGISTER_PAGE_HOST = 0x07,
			REGISTER_PAGE_CAS =  0x08,
			REGISTER_PAGE_DD =   0x09,      //FEC for T-DMB, DAB, FM

			REGISTER_PAGE_FIC =  0x0A,
			REGISTER_PAGE_MSC0 = 0x0B,
			REGISTER_PAGE_MSC1 = 0x0C,
			REGISTER_PAGE_RF =   0x0F
		};
		enum REGISTER {
			REGISTER_PAGE	= 0x03		/* Page */
		};

	public:
	WRadio(std::shared_ptr<USB::Device> dev) : dev(dev) {
		if (!powerup()) {
			std::cerr << "Powerup failed" << std::endl;
		}
	}

	~WRadio() {
	}

typedef	std::pair<uint8_t, uint8_t>	regconfig;
typedef std::vector<regconfig>		regconfigset;

	void registers_write(regconfigset &rset) {
		for(auto reg : rset)
			register_write(reg.first, reg.second);

	}

#define REGISTER_READ_TIMEOUT	100
#define REGISTER_WRITE_TIMEOUT	100
#define USBDEV_ENDPOINT_WRITE	0x02
#define USBDEV_ENDPOINT_READ	0x82

	void register_write(uint8_t r, uint8_t v) {
		std::vector<uint8_t> cmd{0x21, 0x00, 0x00, 0x02, r, v};
		dev->bulk(USBDEV_ENDPOINT_WRITE,  cmd, REGISTER_WRITE_TIMEOUT);
		std::cout << "Register write " << std::setw(2) << std::hex << (int) r << " V " << (int) v << std::endl;
	}

	uint8_t register_read(uint8_t r) {
		std::vector<uint8_t> cmd{0x22, 0x00, 0x01, 0x00, r};
		std::vector<uint8_t> result(5);

		dev->bulk(USBDEV_ENDPOINT_WRITE, cmd, REGISTER_WRITE_TIMEOUT);
		dev->bulk(USBDEV_ENDPOINT_READ, result, REGISTER_READ_TIMEOUT);

		std::cout << "Register read " << std::setw(2) << std::hex << (int) r << " V " << (int) result[4] << std::endl;

		return result[4];
	}

	void switchpage(uint8_t	page) {
		register_write(REGISTER_PAGE, page);
	}

	void config_power(void ) {
#define RTV_IO_1_8V	0x02
		regconfigset	powertype={
			{ 0x54, 0x1C },
			{ 0x52, 0x07 },			/* LDODIG_HT */
			{ 0x30, 0xF0 | (RTV_IO_1_8V << 1) },	/* IOLDOCON__REG */
			{ 0x2F, 0x61 | 0x10 }		/* DCDC_OUTSEL = 0x3, PDDCDC_I2C = 1, PDLDO12_I2C = 0 */
		};

		switchpage(REGISTER_PAGE_RF);
		registers_write(powertype);
	}

	bool powerup(void ) {
		switchpage(REGISTER_PAGE_HOST);

		register_write(0x7d, 0x06);

		for(int i=0;i<5;i++) {
			uint8_t powerstate=register_read(0x7d);
			if (powerstate == 0x06)
				return true;
		}
		return false;
	}

	void run(void ) {
		config_power();
		
	}
};


#endif

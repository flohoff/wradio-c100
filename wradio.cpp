
#include <vector>
#include <memory>
#include <iostream>
#include <string.h>
#include <chrono>
#include <thread>


#include "usb.hpp"
#include "raontunerinput.h"
#include "jdabppprtkservice.h"

int main(void ) {
	USB	usb;
	std::shared_ptr<USB::Device>	usbdev;

	usbdev=usb.device_by_vendprod(0x16c0, 0x05dc);

	if (usbdev == nullptr) {
		std::cerr << "Unable to find usb device" << std::endl;
		exit(-1);
	}

	RaonTunerInput	tuner(usbdev);
	while(!tuner.isInitialized()) {
		tuner.initialize();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	// Adv PPP RTK
	// Bundexmux Kanal 5C
	// Ensemble ID 0x10bc
	// Service ID 0xe0d210bc
	std::shared_ptr<JDabPPPRTKService>	jdab;
	jdab=std::make_shared<JDabPPPRTKService>(178352000, 0xFF, 0x10bc, 0xe0d210bc);

	tuner.startService(jdab);

	//tuner.startServiceScan();

	while(42)
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

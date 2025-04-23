
#include <vector>
#include <memory>
#include <iostream>
#include <string.h>
#include <chrono>
#include <thread>


#include "usb.hpp"
#include "raontunerinput.h"
#include "jdabservice.h"

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

	//tuner.startServiceScan();
	//std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	std::shared_ptr<JDabService>	jdab;
// Service ID 3771863228
// Ensemble 10bc
	//jdab=std::make_shared<JDabService>(178352000, 0xFF, 0x10bc, 3771863228);
	// Adv PPP RTK
	// [DabEnsemble] ServiceSanity FIC 01_05: e0d210bc : PPP-RTK-AdV
	jdab=std::make_shared<JDabService>(178352000, 0xFF, 0x10bc, 0xe0d210bc);

	// Audio?
	//jdab=std::make_shared<JDabService>(178352000, 0xFF, 0x10bc, 0x100d);

	tuner.startService(jdab);

	//tuner.startServiceScan();
	//tuner.tuneFrequency(178352);

	while(42)
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	//wradio.run();
}

#include "jdabppprtkservice.h"
#include "dabservicecomponentmscpacketdata.h"

void JDabPPPRTKService::dataFrameInput(std::shared_ptr<DabDataFrame> frame) {

	std::cout << "Frame callback called - adding bytes" << std::endl
		<< *frame
		<< std::endl;

	rtcm.append(frame->userdata(), frame->usersize());

	std::cout << "Total RTCM Frame buffer" << std::endl
		<< rtcm
		<< std::endl;

	while(42) {
		/* We lost sync - preamble not in buffer */
		if (!rtcm.preamble()) {
			std::cout << "Error - RTCM preamble missing " << std::endl << rtcm << std::endl;
			rtcm.clear();
			break;
		}

		if (!rtcm.complete())
			break;

		std::shared_ptr<RtcmFrame> sub=rtcm.firstframe();
		std::cout << "RTCM Extracted frame" << std::endl
			<< *sub << std::endl;
	}
}

void JDabPPPRTKService::setLinkDabService(std::shared_ptr<DabService> linkedDabSrv) {
	std::cout << m_logTag << "JDabPPPRTKServive Linking DABServices..." << std::endl;
	m_linkedDabService = linkedDabSrv;
	for(const auto& srvComp : linkedDabSrv->getServiceComponents()) {
		std::cout << m_logTag << "Component type " << srvComp->getServiceComponentType() << std::endl;

		/* We only want the packet data component */
		if (srvComp->getServiceComponentType() != DabServiceComponent::SERVICECOMPONENTTYPE::MSC_PACKET_MODE_DATA) {
			continue;
		}

		std::shared_ptr<DabServiceComponentMscPacketData> packetComponent = std::static_pointer_cast<DabServiceComponentMscPacketData>(srvComp);
		m_packetDataCb=packetComponent->registerPacketDataCallback(std::bind(&JDabPPPRTKService::dataFrameInput, this, std::placeholders::_1));
	}
}

JDabPPPRTKService::JDabPPPRTKService(uint32_t freq, uint8_t ecc, uint16_t eid, uint32_t serviceid) :
	JDabService(freq, ecc, eid, serviceid) {
}



#include "jdabppprtkservice.h"
#include "dabservicecomponentmscpacketdata.h"

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

void JDabPPPRTKService::dataFrameInput(std::shared_ptr<DabDataFrame> frame) {
#ifdef DEBUG_JDABPPPRTK
	std::cout << "Frame callback called - adding bytes" << std::endl
		<< *frame
		<< std::endl;
#endif
	rtcm.append(frame->userdata(), frame->usersize());

#ifdef DEBUG_JDABPPPRTK
	std::cout << "Total RTCM Frame buffer" << std::endl
		<< rtcm
		<< std::endl;
#endif

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

		if (!rtcm.preamble()) {

		}

		if (m_ntripServerEnabled)
			m_ntripServer->pushRtcm(sub);
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

void JDabPPPRTKService::enableNtripServer(std::string host, std::string port, std::string user, std::string passowrd, std::string mount) {
	m_ntripServer=std::unique_ptr<ntripServer>(new ntripServer("127.0.0.1", "2101", "flo", "flo", "ZZ-DAB-SSRZ"));
	m_ntripServerEnabled=true;
}

JDabPPPRTKService::JDabPPPRTKService(uint32_t freq, uint8_t ecc, uint16_t eid, uint32_t serviceid) :
	JDabService(freq, ecc, eid, serviceid) {
}



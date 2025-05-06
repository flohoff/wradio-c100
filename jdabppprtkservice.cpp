#include "jdabppprtkservice.h"
#include "dabservicecomponentmscpacketdata.h"

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

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
		m_rtcmQueue.push(sub);
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

void JDabPPPRTKService::NTRIPServer() {
	ntripsocket=socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in ntripcaster;

	struct hostent* host = gethostbyname("127.0.0.1");

	ntripcaster.sin_family = AF_INET;
	ntripcaster.sin_port = htons(2101);
	ntripcaster.sin_addr.s_addr =
		inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));

	while(42) {
		int status = connect(ntripsocket,
			(sockaddr*) &ntripcaster, sizeof(ntripcaster));

		if (status >= 0) {
			break;
		}

		std::cout << "Error connecting socket" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	std::cout << "Connected" << std::endl;

	struct timeval tv_read;
	tv_read.tv_sec = 2;
	tv_read.tv_usec = 0;
	setsockopt(ntripsocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_read, sizeof tv_read);

	char msg[]="SOURCE flo ZZ-DAB-SSRZ\r\nSource-Agent: NTRIP wradio\r\n STR: \r\n\r\n";
	send(ntripsocket, (char*)&msg, strlen(msg), 0);

	char rbuf[1024];
	read(ntripsocket, &rbuf, sizeof(rbuf));

	std::cout << "Returned: " << rbuf << std::endl;

	while(42) {
		std::shared_ptr<RtcmFrame>	frame;
		if(m_rtcmQueue.tryPop(frame, std::chrono::milliseconds(24))) {
			std::cout << "RTCMServer frame" << std::endl << *frame << std::endl;

			send(ntripsocket, frame->data(), frame->size(), 0);
		}
	}
}

JDabPPPRTKService::JDabPPPRTKService(uint32_t freq, uint8_t ecc, uint16_t eid, uint32_t serviceid) :
	JDabService(freq, ecc, eid, serviceid) {

	m_rtcmServer = std::thread(&JDabPPPRTKService::NTRIPServer, this);
}



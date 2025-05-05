#include "jdabppprtkservice.h"
#include "dabservicecomponentmscpacketdata.h"

void JDabPPPRTKService::packetDataInput(const std::vector<uint8_t> &data, int len) {
	//std::cout << "Packet callback called" << std::endl;

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
		m_packetDataCb=packetComponent->registerPacketDataCallback(std::bind(&JDabPPPRTKService::packetDataInput, this, std::placeholders::_1, std::placeholders::_2));
	}
}

JDabPPPRTKService::JDabPPPRTKService(uint32_t freq, uint8_t ecc, uint16_t eid, uint32_t serviceid) :
	JDabService(freq, ecc, eid, serviceid) {
}



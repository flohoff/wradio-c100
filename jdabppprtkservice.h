#ifndef JDABPPPRTKSERVICE_H
#define JDABPPPRTKSERVICE_H

#include "jdabservice.h"
#include "ntripserver.h"
#include "rtcmframe.h"
#include "dabservicecomponentmscpacketdata.h"

class JDabPPPRTKService : public JDabService {
	private:

	std::shared_ptr<DabServiceComponentMscPacketData::DATA_FRAME_CALLBACK> m_packetDataCb{nullptr};
	RtcmFrame		rtcm;

	public:
	JDabPPPRTKService(uint32_t freq, uint8_t ecc, uint16_t eid, uint32_t serviceid);
	virtual void setLinkDabService(std::shared_ptr<DabService> linkedDabSrv);
	void enableNtripServer(std::string, std::string, std::string, std::string, std::string);

	private:
	void dataFrameInput(std::shared_ptr<DabDataFrame> frame);

	bool						m_ntripServerEnabled{false};
	std::unique_ptr<ntripServer>			m_ntripServer;
};

#endif

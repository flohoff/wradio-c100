#ifndef JDABPPPRTKSERVICE_H
#define JDABPPPRTKSERVICE_H

#include "jdabservice.h"
#include "rtcmframe.h"
#include "dabservicecomponentmscpacketdata.h"
#include "concurrent_queue.h"

class JDabPPPRTKService : public JDabService {
	private:

	std::shared_ptr<DabServiceComponentMscPacketData::DATA_FRAME_CALLBACK> m_packetDataCb{nullptr};
	RtcmFrame		rtcm;

	public:
	JDabPPPRTKService(uint32_t freq, uint8_t ecc, uint16_t eid, uint32_t serviceid);
	virtual void setLinkDabService(std::shared_ptr<DabService> linkedDabSrv);

	private:
	void NTRIPServer();
	void dataFrameInput(std::shared_ptr<DabDataFrame> frame);

	ConcurrentQueue<std::shared_ptr<RtcmFrame>>	m_rtcmQueue;
	std::thread					m_rtcmServer;

	int						ntripsocket;
};

#endif

#ifndef DABDATAFEC_H
#define DABDATAFEC_H

#include <cstdint>
#include <vector>
#include <memory>
#include <list>

#include "dabdatapkt.h"

extern "C" {
    #include "thirdparty/fec/fec.h"
}

#define FEC_DATA_SIZE		2256
#define FEC_BUFFER_SIZE		(FEC_COLUMNS*FEC_ROWS)
#define FEC_DATA_COLUMNS	239
#define FEC_COLUMNS		16
#define FEC_ROWS		12
#define FEC_PAD			51
#define FEC_PKT_BYTES		22
#define FEC_PKT_HDR_LENGTH	2
#define FEC_PKT_BYTES		22

class DabDataFec {
	public:
	DabDataFec();
	~DabDataFec();
	bool packetInput(std::shared_ptr<DabDataPkt> pkt);
	std::list<std::shared_ptr<DabDataPkt>>::iterator packetList();
	void packetsClear(void );

	typedef std::list<std::shared_ptr<DabDataPkt>>::iterator iterator;
	inline iterator begin() { return pkts.begin(); }
	inline iterator end() { return pkts.end(); }

	private:
	bool packetsProcessFec(void );

	private:
	int m_fecPosition{0};
	void *rs_handle{nullptr};

	int pktcount{0};
	std::vector<uint8_t> m_fecBuffer;
	std::list<std::shared_ptr<DabDataPkt>>	pkts;
};

#endif

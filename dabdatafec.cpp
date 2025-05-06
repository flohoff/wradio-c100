
#include <cstring>
#include "dabdatafec.h"

DabDataFec::DabDataFec() {
	m_fecBuffer.reserve(FEC_BUFFER_SIZE);
	rs_handle=init_rs_char(8, 0x11d, 0, 1, 16, 0);
}

DabDataFec::~DabDataFec() {
	free_rs_char(rs_handle);
}

void DabDataFec::packetsClear(void ) {
	pkts.clear();
	pktcount=0;
}

typedef CustomHexdump<255, true> FECHexdump;

bool DabDataFec::packetsProcessFec(void ) {
	uint8_t		rstable[FEC_ROWS][FEC_DATA_COLUMNS+FEC_COLUMNS];
	int		dptr=0;		/* Data ptr */
	int		fecpkts=0;
	int		pktbytes=0;
	int		pktcount=0;

#define FEC_PKT_HDR_LENGTH	2
#define FEC_PKT_BYTES		22

	std::memset(rstable, 0, FEC_ROWS*(FEC_DATA_COLUMNS+FEC_COLUMNS));

	for (auto &pkt : pkts) {
		uint8_t	*pbuf=pkt->data();

#ifdef DEBUG_FEC
		std::cout << "packetsProcessFec: Adding packet " << pkt->seq()
			<< " fec " << std::boolalpha << pkt->is_fec()
			<< std::endl;
#endif

		if (pkt->is_fec()) {
			/*
			 * FEC packets (should be 9 in our buffer)
			 * Must be interleaved into columns from column 239 on
			 */
			int poff=pkt->fec_count()*FEC_PKT_BYTES;
			for(int i=0;i<FEC_PKT_BYTES;i++)
				rstable[(poff+i) % FEC_ROWS][FEC_DATA_COLUMNS + (poff+i) / FEC_ROWS]=pbuf[FEC_PKT_HDR_LENGTH+i];

			fecpkts++;
		} else {
			/* Overflowing buffer? */
			if (dptr+pkt->size() > FEC_DATA_SIZE) {
				packetsClear();
				return false;
			}

			/* Data packet - interleave into columns */
			for(size_t i=0;i<pkt->size();i++) {
				rstable[dptr % FEC_ROWS][FEC_PAD + dptr / FEC_ROWS]=pbuf[i];
				dptr++;
			}
			pktbytes+=pkt->size();
			pktcount++;
		}
	}

	if (pktbytes != FEC_DATA_SIZE) {
		std::cout << "packetsProcessFec: Unable to run FEC - did not receive all packets" << std::endl;
		/*
		 * Our guarantee is that all packets arrive twice in the post FEC pipeline.
		 * Once in original form, and once handled by FEC:
		 * In case we really loose packets we may not even issue FEC but we still want
		 * to hold up our guarantee so we we mark packets handled and signal
		 * that packets should be sent again.
		 */
		for (auto &pkt : pkts) {
			pkt->fec_handled_set(true);
		}

		return true;
	}

#ifdef DEBUG_FEC
	std::cout << "packetsProcessFec: FEC pkts " << pktcount
		<< " bytes " << pktbytes
		<< " FEC packets " << fecpkts << std::endl;
	//std::cout << FECHexdump(rstable, FEC_ROWS*(FEC_COLUMNS+FEC_DATA_COLUMNS)) << std::endl;
#endif
	for(unsigned int r=0;r<FEC_ROWS;r++) {
		int corr_pos[10];
		int corr_count=decode_rs_char(rs_handle, rstable[r], corr_pos, 0);

#ifdef DEBUG_FEC
		std::cout << "packetsProcessFec: Row " << r << " " << corr_count << " errors in FEC" << std::endl;
#endif

		/*
		 * We need to copy back to packet buffers in case of corrected bytes
		 * As we copyied them interleaved into the rows we need to walk through
		 * again and if it matches to the corrected position copy back the byte.
		 *
		 */

		// FIXME - Mark packets which may contain uncorrectable errors
		for(int i=0;i<corr_count;i++) {
			dptr=0;
			unsigned int cpos=corr_pos[i];
#ifdef DEBUG_FEC
			std::cout << "packetsProcessFec: Row " << r << " correction position " << cpos << std::endl;
#endif

			for (auto &pkt : pkts) {
				uint8_t	*pbuf=pkt->data();

				/* Data packet - interleave into columns */
				for(size_t j=0;j<pkt->size();j++) {
					if ((dptr % FEC_ROWS == r) && ((FEC_PAD + dptr / FEC_ROWS) == cpos)) {

						uint8_t b=rstable[dptr % FEC_ROWS][FEC_PAD + dptr / FEC_ROWS];

						if (pbuf[j] != b) {
							pkt->fec_bytes_inc();
#ifdef DEBUG_FEC
							std::cout << "DabDataFec: Packet " << pkt->seq() 
								<< " correcting byte " << j
								<< std::endl;

							std::cout << "Before FEC correction" << std::endl
								<< *pkt
								<< std::endl;
#endif

							pbuf[j]=b;
#ifdef DEBUG_FEC
							std::cout << "After FEC correction" << std::endl
								<< *pkt
								<< std::endl;
#endif
						}
					}
					dptr++;
				}
			}
		}

		for (auto &pkt : pkts) {
			pkt->fec_handled_set(true);
		}
	}

	return true;
}

bool DabDataFec::packetInput(std::shared_ptr<DabDataPkt> pkt) {
	pktcount++;
	pkts.push_back(pkt);

	/* We need to issue FEC if we have all 9 FEC frames (0-8) */
	if (pkt->is_fec() && pkt->fec_count() == 8) {
		/* After successful FEC push packets to consumer */
		// FIXME Regardless of the return code - push packets to
		// allow the assumption that WITH FEC all packets appear twice
		if (packetsProcessFec()) {
			return true;
		}
	}
	return false;
}


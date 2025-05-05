#ifndef DABDATAFRAMEAGGREGATOR_H
#define DABDATAFRAMEAGGREGATOR_H

#include <list>
#include <vector>
#include <memory>

#include "dabdatapkt.h"
#include "dabdataframe.h"

/* EN 300 401 V2.1.1
 *
 * 5.3.2 defines a Packet Mode where individual Data Packets contain
 * an address, some sequence numbers and frame begin/end markers.
 *
 * This packet consumer should be attached to an Address Demux so only
 * packets with a defined address reach this input.
 *
 */

/* We will need to:
 * - Check packets continuity bits
 *   - Error mode - wait for FEC or firstlast begin bit
 * - Check packets CRC
 *   - If CRC fails we might get the FEC corrected packet later
 *     so we need to stop process direct packets Until:
 *     - Frame Begin Bit is set - clear state and start with next frame
 *     - FEC frames with the correct Seq Number and CRC comes in and
 *       we try to complete the frame
 * - Wait for Frame begin bit in firstlast
 */

//#define DEBUG_FRAMEAGGREGATOR 1

class DabDataFrameAggregator {
	private:
		uint8_t continuity_last;
		int	numpkts=0;
		std::vector<std::shared_ptr<DabDataPkt>>	pkts;


	public:
		std::shared_ptr<DabDataFrame>	dataframe;

		~DabDataFrameAggregator() {};
		DabDataFrameAggregator() {};

		uint8_t continuity_expected(void ) {
			return ((continuity_last+1)&0x3);
		}

		/* When called we assume the first packet in the list
		 *
		 * to have pkt->frame_first() == true, and the last packet
		 * to have pkt->frame_last() == true
		 *
		 * if this is not the case we simply drop the frame.
		 */
		bool aggregate(void ) {
			uint8_t		cont=pkts[0]->continuity();

			if (!pkts[0]->frame_first()) {
				std::cerr << "First packet in aggregator not marked as first" << std::endl;
				return false;
			}

			// FIXME We want to check last packet to contain ->frame_last() == true

			dataframe=std::make_shared<DabDataFrame>(DabDataFrame());

			for (auto pkt : pkts) {
				// FIXME We want to check CRC and frame continuity and drop frame
				dataframe->append(pkt);
			}

#ifdef DEBUG_FRAMEAGGREGATOR
			std::cout << *dataframe << std::endl;
#endif

			return true;
		}

		std::shared_ptr<DabDataFrame> frame(void ) {
			return dataframe;
		}

		bool input(std::shared_ptr<DabDataPkt> pkt) {
			bool r=false;

#ifdef DEBUG_FRAMEAGGREGATOR
			std::cout << "Address " << pkt->address()
				<< " PktSeq " << pkt->seq()
				<< " FEC handled " << (pkt->fec_handled() ? "Yes" : "No")
				<< " CRC " << (pkt->crc_correct() ? "Valid" : "Invalid")
				<< " Continuity " << (int) pkt->continuity()
				<< " FirstLast " << (int) pkt->frame_firstlast()
				<< std::endl;
#endif
			continuity_last=pkt->continuity();

			if (pkt->frame_first() || pkt->frame_oneandonly()) {
				pkts.clear();
			}

			pkts.push_back(pkt);

			if (pkt->frame_last() || pkt->frame_oneandonly()) {
				r=aggregate();
				pkts.clear();
			}

			return r;
		}
};

#endif

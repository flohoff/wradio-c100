#ifndef DABPKT_H
#define DABPKT_H

#include <vector>
#include <cstdint>
#include <iostream>

#include "hexdump.hpp"
#include "global_definitions.h"


/*
 * EN 300 401 - 5.3.5.2 - FEC for MSC packet Mod
 * Address: this 10-bit field shall take the binary value "1111111110" (1 022).
 */
#define pktaddress(x)   (((x[0]) & 0x3) << 8 | (x[1]))

/*
 * EN 300 401 - 5.3.5.2 - Packet header Counter b13 .. b10
 */
#define pktcounter(x)   ((x[0] >> 2) & 0xf)

/*
 * EN 300 401 - 5.3.2.1 - Packet header Continuity index
 */
#define pktcontinuity(x)   ((x[0] >> 4) & 0x3)

/*
 * EN 300 401 - 5.3.2.1 - Packet header First/Last
 */
#define pktfirstlast(x)   ((x[0] >> 2) & 0x3)

/*
 * EN 300 401 - 5.3.2.0 - Useful data length
 */
#define pktusefuldatalen(x)   (x[2] & 0x7f)


class DabDataPkt {
	private:
		std::vector<uint8_t>	buffer;
		bool			fechandled=false;
		uint8_t			fecbytes=0;

		int			seqno;
	public:
		DabDataPkt(int seqno, const std::vector<uint8_t> &data, int len);
		inline std::size_t size(void ) {
			return buffer.size();
		}

		inline uint8_t *data(void ) {
			return buffer.data();
		}

		inline const std::vector<uint8_t> &data_vector(void ) {
			return buffer;
		}

		inline void fec_handled_set(bool state) {
			fechandled=state;
		}

		inline bool fec_handled(void ) {
			return fechandled;
		}

		inline uint8_t fec_bytes(void ) {
			return fecbytes;
		}

		inline int seq(void ) {
			return seqno;
		}

		inline uint8_t fec_bytes_inc(void ) {
			return fecbytes++;
		}

		/*
		 * EN 300 401 - 5.3.5.2 - FEC for MSC packet Mod
		 * Address: this 10-bit field shall take the binary value "1111111110" (1 022).
		 */
		inline bool is_fec(void ) {
			return (pktaddress(buffer.data()) == 0x3fe);
		};

		inline uint16_t address(void ) {
			return pktaddress(buffer.data());
		}

		inline bool is_empty(void ) {
			return (pktaddress(buffer.data()) == 0);
		}

		inline uint8_t	continuity(void ) {
			return pktcontinuity(buffer.data());
		}

		inline uint8_t data_len(void ) {
			return pktusefuldatalen(buffer.data());
		}

		inline uint8_t	frame_firstlast(void ) {
			return pktfirstlast(buffer.data());
		}

		inline uint8_t frame_first(void ) {
			return pktfirstlast(buffer.data()) == 0x2;
		}

		inline uint8_t frame_oneandonly(void ) {
			return pktfirstlast(buffer.data()) == 0x3;
		}

		inline uint8_t frame_last(void ) {
			return pktfirstlast(buffer.data()) == 0x1;
		}

		inline short fec_count(void ) {
			return pktcounter(buffer.data());
		};

		inline bool crc_correct(void ) {
			return CRC_CCITT_CHECK(buffer.data(), buffer.size());
		}

		friend std::ostream& operator<<(std::ostream& out, const DabDataPkt &pkt);
};


#endif

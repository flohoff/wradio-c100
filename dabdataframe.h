#ifndef DABDATAFRAME_H
#define DABDATAFRAME_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <memory>

#include "dabdatapkt.h"
#include "global_definitions.h"
#include "hexdump.hpp"

class DabDataFrame {
	protected:
		std::vector<uint8_t>	buffer;
	public:
		DabDataFrame(void )  {
		};

		void append(const std::shared_ptr<DabDataPkt> pkt) {
			size_t len=pkt->data_len();
			const std::vector<uint8_t> &a=pkt->data_vector();

			auto begin=std::begin(a);
			std::advance(begin, 3);

			auto end=std::begin(a);
			std::advance(end, 3+len);

			std::copy(begin, end, std::back_inserter(buffer));
		}

		std::size_t size(void ) {
			return buffer.size();
		}

		uint8_t *data(void ) {
			return buffer.data();
		}

		bool dg_crc_correct(void ) {
			return CRC_CCITT_CHECK(buffer.data(), buffer.size());
		}

#define DG_EXTENSION_FLAG	0x80
#define DG_CRC_FLAG		0x40
#define DG_SEGMENT_FLAG		0x20
#define DG_USERACCESS_FLAG	0x10

		bool dg_has_crc(void ) const {
			/* EN 300 401 V2.1.1 5.3.3.0 */
			return (buffer.data()[0] & DG_CRC_FLAG) != 0;
		}

		bool dg_has_segment(void ) const {
			/* EN 300 401 V2.1.1 5.3.3.0 */
			return (buffer.data()[0] & DG_SEGMENT_FLAG) != 0;
		}

		bool dg_has_useraccess(void ) const {
			/* EN 300 401 V2.1.1 5.3.3.0 */
			return (buffer.data()[0] & DG_USERACCESS_FLAG) != 0;
		}

		bool dg_has_extension(void ) const {
			/* EN 300 401 V2.1.1 5.3.3.0 */
			return (buffer.data()[0] & DG_EXTENSION_FLAG) != 0;
		}

		uint8_t dg_continuity(void ) const {
			/* EN 400 401 V2.1.1 5.3.3.1 */
			return (buffer.data()[1] & 0xf0) >> 4;
		}

		uint8_t dg_repetition(void ) const {
			/* EN 400 401 V2.1.1 5.3.3.1 */
			return (buffer.data()[1] & 0xf);
		}

		friend std::ostream& operator<<(std::ostream& out, const DabDataFrame &frame) {
			return out
				<< std::boolalpha
				<< " Continuity " << (int) frame.dg_continuity()
				<< " Length " << frame.buffer.size()
				<< " extension " << frame.dg_has_extension()
				<< " useraccess " << frame.dg_has_useraccess()
				<< " segment " << frame.dg_has_segment()
				<< std::endl
				<< Hexdump((const void *) frame.buffer.data(), frame.buffer.size());
		}
};

std::ostream& operator<<(std::ostream& out, const DabDataFrame &frame);

#endif

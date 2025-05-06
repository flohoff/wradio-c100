
#include "dabdatapkt.h"
#include "hexdump.hpp"
#include <iostream>

DabDataPkt::DabDataPkt(int seqno, const std::vector<uint8_t> &data, int len) : seqno(seqno) {
	buffer.insert(buffer.begin(), data.begin(), data.begin()+len);
}

std::ostream& operator<<(std::ostream& out, const DabDataPkt &pkt) {
	return out
		<< "Length " << pkt.buffer.size()
		<< " Seq " << pkt.seq()
		<< " CRC Valid " << std::boolalpha << pkt.crc_correct()
		<< " Continuity " << std::dec << (int) pkt.continuity()
		<< " FEC Handled " << std::dec << (int) pkt.fec_handled()
		<< " First " << std::boolalpha << pkt.frame_first()
		<< " Last " << std::boolalpha << pkt.frame_last()
		<< " Only " << std::boolalpha << pkt.frame_oneandonly()
		<< std::endl
		<< Hexdump((const void *) pkt.buffer.data(), pkt.buffer.size());
}




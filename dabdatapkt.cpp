
#include "dabdatapkt.h"
#include "hexdump.hpp"
#include <iostream>

DabDataPkt::DabDataPkt(int seqno, const std::vector<uint8_t> &data, int len) : seqno(seqno) {
	buffer.insert(buffer.begin(), data.begin(), data.begin()+len);
}

std::ostream& operator<<(std::ostream& out, const DabDataPkt &pkt) {
	return out << "Length " << pkt.buffer.size() << std::endl
		<< Hexdump((const void *) pkt.buffer.data(), pkt.buffer.size());
}




#ifndef RTCMFRAME_H
#define RTCMFRAME_H

#include <cstdint>
#include <vector>
#include <iostream>
#include <memory>

#include "hexdump.hpp"

#define RTCM_PREAMBLE	0xd3

extern "C" {
    #include "crc24q.h"
}

class RtcmFrame {
	private:
		std::vector<uint8_t>	buffer;
	public:

	inline void append(uint8_t *framebuffer, std::size_t len) {
		for(int i=0;i<len;i++)
			buffer.push_back((uint8_t) framebuffer[i]);
	}

	bool preamble(void ) const {
		return (buffer[0] == RTCM_PREAMBLE);
	}

	void clear(void ) {
		buffer.clear();
	}

	size_t buffersize(void ) const {
		return buffer.size();
	}

#define RTCM_CRC_BYTES	3
#define RTCM_HDR_BYTES	3

	size_t length(void ) const {
		return ((buffer[1]&0x3)<<8)|(buffer[2])+RTCM_HDR_BYTES+RTCM_CRC_BYTES;
	}

	bool complete(void ) const {
		return buffersize() >= length();
	}

	std::shared_ptr<RtcmFrame> firstframe(void ) {
		std::shared_ptr<RtcmFrame>	first(new RtcmFrame);

		first->append(buffer.data(), length());
		buffer.erase(buffer.begin(), buffer.begin()+length());

		return first;
	}

	bool crc_valid(void ) const {
		return crc24q_check(buffer.data(), length());
	}

	friend std::ostream& operator<<(std::ostream& out, const RtcmFrame &frame) {
		return out
			<< "First Frame Length " << frame.length()
			<< " Buffer length " << frame.buffer.size()
			<< " CRC valid " << std::boolalpha << frame.crc_valid()
			<< std::endl
			<< Hexdump((const void *) frame.buffer.data(), frame.buffer.size())
			<< std::endl;
	}
};

std::ostream& operator<<(std::ostream& out, const RtcmFrame &frame);

#endif

/*
 * Copyright (C) 2018 IRT GmbH
 *
 * Author:
 *  Fabian Sattler
 *
 * This file is a part of IRT DAB library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#include "dabservicecomponentmscpacketdata.h"

#include <iostream>

#include "global_definitions.h"
#include "hexdump.hpp"

#include "mscdatagroupdecoder.h"

constexpr uint8_t DabServiceComponentMscPacketData::PACKETLENGTH[4][2];

DabServiceComponentMscPacketData::DabServiceComponentMscPacketData() {
    m_componentType = DabServiceComponent::SERVICECOMPONENTTYPE::MSC_PACKET_MODE_DATA;
}

DabServiceComponentMscPacketData::~DabServiceComponentMscPacketData() {

}

uint16_t DabServiceComponentMscPacketData::getDataServiceComponentId() const {
    return m_serviceComponentId;
}

uint8_t DabServiceComponentMscPacketData::getDataServiceComponentType() const {
    return m_dscty;
}

uint16_t DabServiceComponentMscPacketData::getPacketAddress() const {
    return m_packetAddress;
}

uint16_t DabServiceComponentMscPacketData::getCaOrganization() const {
    return m_caOrg;
}

bool DabServiceComponentMscPacketData::isDataGroupTransportUsed() const {
    return m_dataGroupsUsed;
}

void DabServiceComponentMscPacketData::setDataServiceComponentId(uint16_t scid) {
    m_serviceComponentId = scid;
}

void DabServiceComponentMscPacketData::setPacketAddress(uint16_t packAddr) {
    m_packetAddress = packAddr;
}

void DabServiceComponentMscPacketData::setCaOrganization(uint16_t caOrg) {
    m_caOrg = caOrg;
}

void DabServiceComponentMscPacketData::setIsDataGroupTransportUsed(bool dataGroupsUsed) {
    m_dataGroupsUsed = dataGroupsUsed;
}

void DabServiceComponentMscPacketData::setDataServiceComponentType(uint8_t dscty) {
    m_dscty = dscty;
}

void DabServiceComponentMscPacketData::flushBufferedData() {
}

void DabServiceComponentMscPacketData::applyFec(const std::vector<uint8_t>& pkt, int len) {

}


/* EN 300 401 V2.1.1 5.3.2.0 Packet Header */
#define pktLength(x) (((((x[0])>>6)&0x3)+1)*24)

/* EN 300 401 V2.1.1 5.3.2.0 Packet Header Address */
#define pktAddress(x) (((x[0])&0x3)<<8|(x[1]))

void DabServiceComponentMscPacketData::packetInput(const std::vector<uint8_t>& pkt, int len) {
	std::cout << "Packet len " << len
		<< std::endl << Hexdump(m_unsyncDataBuffer.data(), len) << std::endl;

	if (m_fecSchemeAplied) {
		applyFec(pkt, len);
		return;
	}
}

void DabServiceComponentMscPacketData::packetReframe(const std::vector<uint8_t>& mscData) {

	/* Overflow? */
	m_unsyncDataBuffer.insert(m_unsyncDataBuffer.end(), mscData.begin(), mscData.end());

	/*
	 * We cant use CRC for packet identification here as bytes may be corrupt
	 * and we potentially could correct them with FEC - So using CRC
	 * here for reframing makes the FEC useless.
	 *
	 */
	while(42) {
		/* EN 300 401 - Signals packet size 24, 48, 72, 96 bytes */
		int	len=pktLength(m_unsyncDataBuffer.data());

		/* Do we have the full packet? */
		if (m_unsyncDataBuffer.size() <= len)
			break;

#define PKT_ADDRESS_FEC 0x3fe

		/* FEC packets dont have a CRC - so dont try to make any sense of them */
		if (pktAddress(m_unsyncDataBuffer.data()) != PKT_ADDRESS_FEC) {
			if(!CRC_CCITT_CHECK(m_unsyncDataBuffer.data(), len)) {
				m_crcfail++;
			} else {
				m_crcfail=0;
			}
		}

		/*
		 * 100 consecutive packets failed CRC so we assume out of sync by MSC buffer zap, chip reset whatever.
		 * As FEC will also not work or correct the errors we should reset the FEC aswell
		 *
		 * We now start to drop bytes upfront until we find a valid CRC
		 *
		 */
		if (m_crcfail > 100) {
			m_unsyncDataBuffer.erase(m_unsyncDataBuffer.begin(), m_unsyncDataBuffer.begin()+1);
			continue;
		}

		packetInput(m_unsyncDataBuffer, len);

		/* Remove packet from the begin of vector */
		m_unsyncDataBuffer.erase(m_unsyncDataBuffer.begin(), m_unsyncDataBuffer.begin()+len);
	}
}

void DabServiceComponentMscPacketData::componentMscDataInput(const std::vector<uint8_t>& mscData) {
	packetReframe(mscData);
	return;
#if 0
    if(CRC_CCITT_CHECK(mscData.data(), mscData.size())) {

        auto packetIter = mscData.begin();
        uint8_t packetLength = (*packetIter & 0xC0) >> 6;
        uint8_t continuityIndex = (*packetIter & 0x30) >> 4;
        DabServiceComponentMscPacketData::FIRST_LAST_PACKET firstLast = static_cast<DabServiceComponentMscPacketData::FIRST_LAST_PACKET>((*packetIter & 0x0C) >> 2);
        uint16_t packetAddress = (*packetIter++ & 0x03) << 8 | (*packetIter++ & 0xFF);
        bool commandFlag = (*packetIter & 0x80) >> 7;
        uint8_t usefulDataLength = (*packetIter++ & 0x7F);

        if(PACKETLENGTH[packetLength][0] != mscData.size()) {
            return;
        }

        if(m_packetAddress != packetAddress) {
            //May be possible that more than one packetaddresses are transmitted in the same subchannel
            return;
        }

        if(m_dataGroupsUsed) {
            switch (firstLast) {
                case FIRST_LAST_PACKET::FIRST: {
                    //std::cout << m_logTag << " #### Packet First " << std::endl;
                    m_mscPacket.packetData.clear();
                    m_mscPacket.packetData.insert(m_mscPacket.packetData.end(), packetIter, packetIter+usefulDataLength);
                    m_mscPacket.packetAddress = packetAddress;
                    m_mscPacket.continuityIndex = continuityIndex;

                    break;
                }
                case FIRST_LAST_PACKET::INTERMEDIATE: {
                    if(!m_mscPacket.packetData.empty()) {
                        if( ((m_mscPacket.continuityIndex+1) % 4) != continuityIndex) {
                            //std::cout << m_logTag << " Continuity at intermediate packet interrupted: " << +m_mscPacket.continuityIndex << " : " << +continuityIndex << std::endl;

                            //m_mscPacket.packetData.clear();
                            return;
                        }

                        //std::cout << m_logTag << " #### Packet Continuation " << std::endl;
                        m_mscPacket.continuityIndex = continuityIndex;
                        m_mscPacket.packetData.insert(m_mscPacket.packetData.end(), packetIter, packetIter+usefulDataLength);
                    }
                    break;
                }
                case FIRST_LAST_PACKET::LAST: {
                    if(!m_mscPacket.packetData.empty()) {
                        if( ((m_mscPacket.continuityIndex+1) % 4) != continuityIndex) {
                            //std::cout << m_logTag << " Continuity at last packet interrupted: " << +m_mscPacket.continuityIndex << " : " << +continuityIndex << std::endl;

                            //m_mscPacket.packetData.clear();
                            return;
                        }

                        m_mscPacket.continuityIndex = continuityIndex;
                        m_mscPacket.packetData.insert(m_mscPacket.packetData.end(), packetIter, packetIter+usefulDataLength);

                        //std::cout << m_logTag << " #### Packet complete: " << m_mscPacket.packetData.size() << std::endl;

                        if(CRC_CCITT_CHECK(m_mscPacket.packetData.data(), m_mscPacket.packetData.size())) {
                            //std::cout << m_logTag << " #### Packet CRC okay!" << std::endl;

                            enum MOT_DATAGROUP_TYPE {
                                GENERAL_DATA,
                                CA_MESSAGE,
                                MOT_HEADER,
                                MOT_BODY_UNSCRAMBLED,
                                MOT_BODY_SCRAMBLED,
                                MOT_DIRECTORY_UNCOMPRESSED,
                                MOT_DIRECTORY_COMPRESSED,
                            };

                            //MSC Datagroup decoding
                            auto dgIter = m_mscPacket.packetData.begin();
                            //MSC Datagroup Header
                            bool extensionFlag = (*dgIter & 0x80) >> 7;
                            bool crcFlag = (*dgIter & 0x40) >> 6;
                            bool segmentFlag = (*dgIter & 0x20) >> 5;
                            bool userAccessFlag = (*dgIter & 0x10) >> 4;
                            uint8_t datagroupType = (*dgIter++ & 0x0F);

                            uint8_t continuityIdx = (*dgIter & 0xF0) >> 4;
                            uint8_t repetitionIdx = (*dgIter++ & 0x0F);
                            if(extensionFlag) {
                                dgIter += 2;
                            }
                            //std::cout << m_logTag << " MOT DG ExtFlg: " << std::boolalpha << extensionFlag << " CRCFlg: " << crcFlag << " SegFlg: " << segmentFlag << std::noboolalpha << std::endl;
                            //std::cout << m_logTag << " MOT DG Type: " << +datagroupType << " Cont: " << +continuityIdx << " Rep: " << +repetitionIdx << std::endl;

                            //MSC Datagroup Session header
                            if(segmentFlag) {
                                bool isLast = (*dgIter & 0x80) >> 7;
                                uint16_t segNum = (*dgIter++ & 0x7F) << 8 | (*dgIter++ & 0xFF);

                                //std::cout << m_logTag << " MOT DG isLast: " << std::boolalpha << isLast << std::noboolalpha << " SegNum: " << +segNum << std::endl;
                                //if(isLast) {
                                //    std::cout << m_logTag << " MOT DG isLast ##############################" << std::endl;
                                //}
                            }

                            if(userAccessFlag) {
                                uint8_t rfa = (*dgIter & 0xE0) >> 6;
                                bool transportIdFlag = (*dgIter & 0x10) >> 4;
                                uint8_t lengthIndicator = (*dgIter++ & 0x0F);
                                //std::cout << m_logTag << " MOT DG TransportIdFlg: " << std::boolalpha << transportIdFlag << std::noboolalpha << " Length: " << +lengthIndicator << std::endl;
                                if(transportIdFlag) {
                                    uint16_t transportId = (*dgIter++ & 0xFF) << 8 | (*dgIter++ & 0xFF);
                                    //std::cout << m_logTag << " MOT DG TransportId: " << std::hex << transportId << std::dec << std::endl;
                                }
                                std::vector<uint8_t> endUserAddress;
                                for(uint8_t i = 0; i < (lengthIndicator-2); i++) {
                                    //std::cout << m_logTag << " ############## MOT DG EnduserAddress: " << std::hex << *dgIter << std::dec << std::endl;
                                    endUserAddress.push_back(*dgIter++);
                                }
                            }
                        } else {
                            //std::cout << m_logTag << " #### Packet CRC failed!" << std::endl;
                        }
                    }
                    break;
                }
                case FIRST_LAST_PACKET::ONE_AND_ONLY: {

                    break;
                }
            default:
                break;
            }
        }
        //std::cout << m_logTag << " ############################ MSC Packetdata DGUsed: " << std::boolalpha << m_dataGroupsUsed << std::noboolalpha << " Length: " << +mscData.size() << " : " << +DabServiceComponentMscPacketData::PACKETLENGTH[packetLength][0] << " - " << +usefulDataLength << " FirstLast: " << +firstLast << " for SubchanId: " << std::hex << +m_subChanId << std::dec <<  " Cont: " << +continuityIndex << " Address: " << +m_packetAddress << " : " << +packetAddress << " UApps: " << +m_userApplications.size() << std::endl;
    }
#endif
}

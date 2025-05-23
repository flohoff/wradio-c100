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
#include "dabdatapkt.h"
#include "dabdataframe.h"
#include "dabdataframeaggregator.h"


constexpr uint8_t DabServiceComponentMscPacketData::PACKETLENGTH[4][2];

DabServiceComponentMscPacketData::DabServiceComponentMscPacketData() {
    m_componentType = DabServiceComponent::SERVICECOMPONENTTYPE::MSC_PACKET_MODE_DATA;
}

DabServiceComponentMscPacketData::~DabServiceComponentMscPacketData() {
}

std::shared_ptr<DabServiceComponentMscPacketData::DATA_FRAME_CALLBACK> DabServiceComponentMscPacketData::registerPacketDataCallback(DabServiceComponentMscPacketData::DATA_FRAME_CALLBACK cb) {
	return m_dataFrameDispatcher.add(cb);
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

#define PKT_ADDRESS_FEC 0x3fe

/* EN 300 401 - 5.3.5.2 - Packet header Counter b13 .. b10 */
#define pktCounter(x)   ((x[0] >> 2) & 0xf)

/* EN 300 401 V2.1.1 5.3.2.0 Packet Header */
#define pktLength(x) (((((x[0])>>6)&0x3)+1)*24)

/* EN 300 401 V2.1.1 5.3.2.0 Packet Header Address */
#define pktAddress(x) (((x[0])&0x3)<<8|(x[1]))

void DabServiceComponentMscPacketData::frameAggregate(std::shared_ptr<DabDataPkt> pkt) {
	/* We only want our packet service address */
	if (pkt->address() != m_packetAddress) {
		return;
	}

	if (aggregator.input(pkt)) {
		m_dataFrameDispatcher.invoke(aggregator.frame());
	}
}

void DabServiceComponentMscPacketData::packetDeduplicate(std::shared_ptr<DabDataPkt> pkt) {
#ifdef DEBUG_PKTDEDUPE
	std::cout << "packetDeduplicate: Packet enter seq " << pkt->seq()
		<< " length " << (int) pkt->length()
		<< " size " << (int) pkt->size()
		<< " plugged " << std::boolalpha << plugged
		<< " isfec " << pkt->is_fec()
		<< " address " << pkt->address()
		<< " continuity " << (int) pkt->continuity()
		<< " fec handled " << pkt->fec_handled()
		<< " crc ok " << std::boolalpha << pkt->crc_correct()
		<< std::endl;
#endif
	/* Null/PAD packets or FEC packets - no need to process */
	if (pkt->address() == 0 || pkt->is_fec()) {
		return;
	}

	/* Initialization - we drop the first packet and store its sequence and continuity */
	if (seq_last == 0) {
		continuity_last=pkt->continuity();
		seq_last=pkt->seq();

#ifdef DEBUG_PKTDEDUPE
		std::cout << "packetDeduplicate: Dropping packet " << pkt->seq() << " init seq_last 0" << std::endl;
#endif

		return;
	}

	/*
	 * Only packets this Service Component is initialized for - continuity counter
	 * is per address so we need to drop others here.
	 *
	 * FIXME - Packet address may be broken and fixed by FEC
	 */
	if (pkt->address() != m_packetAddress) {
#ifdef DEBUG_PKTDEDUPE
		std::cout << "packetDeduplicate: Dropping packet " << pkt->seq() << " not adddress " << std::endl;
		return;
#endif
	}

	if (plugged) {
		/* We wait for FEC packets */
		if (!pkt->fec_handled()) {
#ifdef DEBUG_PKTDEDUPE
			std::cout << "packetDeduplicate: Dropping packet " << pkt->seq()
				<< " plugged and non FEC"
				<< std::endl;
#endif
			return;
		}

		/*
		 * FIXME - What happens when FEC is zapped because of uncorrectables?
		 * We will possibly not see the same seq again
		 * FIXME - What happens on sequence number wrap
		 */
		if (pkt->seq() <= seq_last) {
#ifdef DEBUG_PKTDEDUPE
			std::cout << "packetDeduplicate: Dropping packet " << pkt->seq()
				<< " plugged, non fec and newer packet "
				<< std::endl;
#endif
			return;
		}

#ifdef DEBUG_PKTDEDUPE
		std::cout << "packetDeduplicate: Packet " << pkt->seq()
			<< " forward - unplugged"
			<< std::endl;
#endif

		/* The packet came again after the FEC */
		frameAggregate(pkt);

		continuity_last=pkt->continuity();
		seq_last=pkt->seq();

		plugged=false;
		return;
	}

	/* We have already send packets up to this - so we can "skip" aka drop them */
	if (pkt->seq() <= seq_last) {
#ifdef DEBUG_PKTDEDUPE
		std::cout << "packetDeduplicate: Dropping packet " << pkt->seq()
			<< " old packet "
			<< " < " << seq_last
			<< std::endl;
#endif
		return;
	}

	if (pkt->fec_handled() ||
		(pkt->crc_correct() && pkt->continuity() == ((continuity_last+1)&0x3))) {

#ifdef DEBUG_PKTDEDUPE
		std::cout << "packetDeduplicate: Packet " << pkt->seq()
			<< " forward - ok or post-fec"
			<< std::endl;
#endif
		frameAggregate(pkt);

		continuity_last=pkt->continuity();
		seq_last=pkt->seq();

		return;
	}

	//seq_last=pkt->seq();
	plugged=true;

#ifdef DEBUG_PKTDEDUPE
	std::cout << "packetDeduplicate: Dropping packet " << pkt->seq()
		<< " plugging " << std::endl
		<< *pkt
		<< std::endl;
#endif

	return;
}

void DabServiceComponentMscPacketData::packetInput(std::shared_ptr<DabDataPkt> pkt) {
	/*
	 * If we dont have fec enabled on this packet data
	 * hannel - just try frame aggregation
	 */
	if (!m_fecSchemeAplied) {
		if (!pkt->is_fec() && !pkt->is_padding())
			frameAggregate(pkt);
		return;
	}

	/* We try to be fast so we forward packets immediatly.
	 * We also try to do FEC - if thats succeeds we forward
	 * packets again so let the following pipelines either
	 * wait for FEC or process immediatly.
	 *
	 * To make it easy for following pipelines we deduplicate the
	 * packet stream afterwards so packets will be only forwarded
	 * once.
	 */
	if (!pkt->is_fec() && !pkt->is_padding())
		packetDeduplicate(pkt);

	if (fec.packetInput(pkt)) {
		for(auto &pkt : fec) {
			if (!pkt->is_fec() && !pkt->is_padding())
				packetDeduplicate(pkt);
		}
		fec.packetsClear();
	}
}

void DabServiceComponentMscPacketData::packetSynchronize(const std::vector<uint8_t>& mscData) {

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
			std::cout << "CRC failed x consecutive packets - may be out of sync - scanning buffer for correct packet" << std::endl;
			m_unsyncDataBuffer.erase(m_unsyncDataBuffer.begin(), m_unsyncDataBuffer.begin()+1);
			continue;
		}

		/*
		 * Create a DabDataPkt and add a sequence no. The sequence number is needed
		 * for post FEC pipelines to do packet deduplication easily
		 */
#ifdef DEBUG_PKTSYNC
		std::cout << "packetSynchronize: Packet seq " << m_seqno
			<< " len " << len
			<< " buffer.size " << m_unsyncDataBuffer.size()
			<< std::endl;
#endif

		/* FIXME Optimization - we could only move the left over bytes and not after every packet */
		packetInput(std::make_shared<DabDataPkt>(DabDataPkt(m_seqno++, m_unsyncDataBuffer, len)));

		/* Remove packet from the begin of vector */
		m_unsyncDataBuffer.erase(m_unsyncDataBuffer.begin(), m_unsyncDataBuffer.begin()+len);
	}
}

void DabServiceComponentMscPacketData::componentMscDataInput(const std::vector<uint8_t>& mscData) {
	packetSynchronize(mscData);
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

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

#ifndef DABSERVICECOMPONENTMSCPACKETDATA_H
#define DABSERVICECOMPONENTMSCPACKETDATA_H

#include "dabservicecomponent.h"
#include "callbackhandle.h"

class DabServiceComponentMscPacketData : public DabServiceComponent {

public:
    explicit DabServiceComponentMscPacketData();
    virtual ~DabServiceComponentMscPacketData();

    virtual uint16_t getDataServiceComponentId() const;
    virtual uint8_t  getDataServiceComponentType() const;
    virtual uint16_t getPacketAddress() const;
    virtual uint16_t getCaOrganization() const;
    virtual bool isDataGroupTransportUsed() const;

    virtual void setDataServiceComponentId(uint16_t scid);
    virtual void setPacketAddress(uint16_t packAddr);
    virtual void setCaOrganization(uint16_t caOrg);
    virtual void setIsDataGroupTransportUsed(bool dataGroupsUsed);
    virtual void setDataServiceComponentType(uint8_t dscty);

    virtual void componentMscDataInput(const std::vector<uint8_t>& mscData) override;
    virtual void flushBufferedData() override;

    using PACKET_DATA_CALLBACK = std::function<void (const std::vector<uint8_t>&, int)>;
    virtual std::shared_ptr<DabServiceComponentMscPacketData::PACKET_DATA_CALLBACK> registerPacketDataCallback(DabServiceComponentMscPacketData::PACKET_DATA_CALLBACK cb);

private:
    void packetSynchronize(const std::vector<uint8_t>& mscData);
    void packetInput(const std::vector<uint8_t>& pkt, int len);
    void applyFec(const std::vector<uint8_t>& pkt, int len);
    void frameAggregate(const std::vector<uint8_t>& pkt, int len);

    CallbackDispatcher<PACKET_DATA_CALLBACK> m_packetDataDispatcher;

private:
    static constexpr uint8_t PACKETLENGTH[4][2] {
        //PacketLength / Packet datafield (useful) length
        {24, 19},
        {48, 43},
        {72, 67},
        {96, 91}
    };

    enum FIRST_LAST_PACKET {
        INTERMEDIATE,
        LAST,
        FIRST,
        ONE_AND_ONLY
    };

    struct MscPacketData {
        uint8_t continuityIndex;
        DabServiceComponentMscPacketData::FIRST_LAST_PACKET firstLast;
        uint16_t packetAddress;
        bool commandFlag;
        std::vector<uint8_t> packetData;
    };

private:
    std::string m_logTag = "[DabServiceComponentMscPacketData]";

    uint16_t m_serviceComponentId{0xFFFF};
    uint8_t m_dscty{0xFF};
    uint16_t m_packetAddress{0xFFFF};
    uint16_t m_caOrg{0x0000};
    bool m_dataGroupsUsed{false};

    MscPacketData m_mscPacket;
    int m_crcfail{0};

#define FEC_DATA_SIZE		2256
#define FEC_BUFFER_SIZE		(FEC_COLUMNS*FEC_ROWS)
#define FEC_DATA_COLUMNS	239
#define FEC_COLUMNS		255
#define FEC_ROWS		12
#define FEC_PAD			51
#define FEC_PKT_BYTES		22

    int m_fecPosition{0};
    void *rs_handle;

    std::vector<uint8_t> m_fecBuffer;
    std::vector<uint8_t> m_unsyncDataBuffer;
};

#endif // DABSERVICECOMPONENTMSCPACKETDATA_H

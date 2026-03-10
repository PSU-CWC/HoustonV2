#pragma once
#include "../dashboard/Dashboard.h"
#include "TypedConsumer.h"

class LiveDataConsumer : public TypedConsumer<std::vector<LiveDataPacket_t>> {
protected:
    std::vector<LiveDataPacket_t> decode(std::vector<uint8_t> &data) override {
        std::vector<LiveDataPacket_t> packets;
        
        // 1. Basic size check (Header + Tail)
        if (data.size() < sizeof(DashboardPacketHeader_t) + sizeof(DashboardPacketTail_t)) {
            return packets;
        }

        DashboardPacketHeader_t header;
        std::memcpy(&header, data.data(), sizeof(DashboardPacketHeader_t));

        // 2. ONLY decode if this is actually a LiveData response
        if (header.packetType != ID_Response_LiveData) {
            return packets; 
        }

        // 3. Calculate how many packets are actually in the payload
        // We subtract the Header and any Key/Tail overhead
        uint32_t payloadStart = sizeof(DashboardPacketHeader_t) + header.payloadKeySize;
        uint32_t packetCount = header.payloadValueSize / sizeof(LiveDataPacket_t);

        for (uint32_t i = 0; i < packetCount; i++) {
            LiveDataPacket_t packet;
            size_t offset = payloadStart + (i * sizeof(LiveDataPacket_t));
            
            // Safety: Ensure we don't read past the end of the vector (ignoring the Tail)
            if (offset + sizeof(LiveDataPacket_t) <= data.size() - sizeof(DashboardPacketTail_t)) {
                std::memcpy(&packet, data.data() + offset, sizeof(LiveDataPacket_t));
                packets.push_back(packet);
            }
        }
        return packets;
    }
};
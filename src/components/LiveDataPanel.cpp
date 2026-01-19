
#include "LiveDataPanel.h"
#include "../consumer/TypedConsumer.h"


LiveDataPanel::~LiveDataPanel() {

}

void LiveDataPanel::start() {
    refreshPackets();
}

void LiveDataPanel::refreshPackets() {
    liveDataMap.clear();
    intMap.clear();
    floatMap.clear();
    DashboardPacketHeader_t header;
    craftDashboardHeaderPacket(&header, ID_Request_LiveData, 0, 0, 0);
    DashboardPacketTail_t tail;
    tail.payloadChecksum = 0;
    std::vector<uint8_t> buffer;
    buffer.resize(sizeof(DashboardPacketHeader_t) + sizeof(DashboardPacketTail_t));
    std::memcpy(buffer.data(), &header, sizeof(DashboardPacketHeader_t));
    std::memcpy(buffer.data() + sizeof(DashboardPacketHeader_t), &tail, sizeof(DashboardPacketTail_t));
    dispatcher->sendData(std::move(buffer));
}

void LiveDataPanel::sendModifyPacket(uint8_t packetID, uint16_t valueType) {
    DashboardPacketHeader_t header;
    craftDashboardHeaderPacket(&header, ID_Modify, valueType, sizeof(LiveDataPacket_t), 0);

    LiveDataPacket_t payload;
    payload.packetID = packetID;
    payload.valueType = valueType;
    if (valueType == TYPE_FLOAT) {
        payload.floatValue = *floatMap[packetID];
    } else if (valueType == TYPE_BOOL) {
        payload.boolValue = *reinterpret_cast<bool *>(intMap[packetID]);
    } else {
        payload.int32Value = *intMap[packetID];
    }
    DashboardPacketTail_t tail;
    tail.payloadChecksum = crc32(reinterpret_cast<char *>(&payload), sizeof(LiveDataPacket_t), 0);
    std::vector<uint8_t> buffer;
    buffer.resize(sizeof(DashboardPacketHeader_t) + sizeof(LiveDataPacket_t) + sizeof(DashboardPacketTail_t));
    std::memcpy(buffer.data(), &header, sizeof(DashboardPacketHeader_t));
    std::memcpy(buffer.data() + sizeof(DashboardPacketHeader_t), &payload, sizeof(LiveDataPacket_t));
    std::memcpy(buffer.data() + sizeof(DashboardPacketHeader_t) + sizeof(LiveDataPacket_t), &tail,
                sizeof(DashboardPacketTail_t));
    dispatcher->sendData(std::move(buffer));
}

void LiveDataPanel::render() {
    // 1. Process Incoming Data from the Consumer Queue
    DataConsumer *consumer = dispatcher->getHandler(ID_Response_LiveData);
    if (consumer) {
        auto *liveDataConsumer = static_cast<TypedConsumer<std::vector<LiveDataPacket_t>> *>(consumer);
        
        while (!liveDataConsumer->isEmpty()) {
            auto dataOpt = liveDataConsumer->pop();
            if (!dataOpt.has_value()) continue;

            // Get reference to the vector (No asterisk needed here)
            auto& packetVector = dataOpt.value(); 
            
            for (auto &packet : packetVector) {
                // If we haven't seen this ID before, initialize our maps
                if (liveDataMap.find(packet.packetID) == liveDataMap.end()) {
                    liveDataMap[packet.packetID] = packet;

                    if (packet.valueType == TYPE_FLOAT) {
                        floatMap[packet.packetID] = new float(packet.floatValue);
                    } else if (packet.valueType == TYPE_BOOL) {
                        // Store bool as 0 or 1 in the intMap for memory stability
                        intMap[packet.packetID] = new int32_t(packet.boolValue ? 1 : 0);
                    } else {
                        // Standard Integer or Unsigned Integer
                        intMap[packet.packetID] = new int32_t(packet.int32Value);
                    }
                } else {
                    // Update existing metadata (like the label or type) if it changed
                    liveDataMap[packet.packetID] = packet;
                }
            }
        }
    }

    // 2. Render the ImGui Window
    ImGui::Begin("Live Data");

    if (liveDataMap.empty()) {
        ImGui::TextDisabled("No live variables registered.");
        ImGui::TextDisabled("Check ESP32 connection and press Refresh.");
    }

    for (auto &pair : liveDataMap) {
        uint8_t id = pair.first;
        LiveDataPacket_t &packet = pair.second;
        
        // --- THIS RESTORES THE LABELS ---
        const char* label = packet.label;

        // Use PushID so multiple "Apply" buttons don't conflict internally
        ImGui::PushID(id); 

        if (packet.valueType == TYPE_BOOL) {
            // Checkbox logic: casting the int32 memory to bool for ImGui
            if (ImGui::Checkbox(label, reinterpret_cast<bool*>(intMap[id]))) {
                sendModifyPacket(id, packet.valueType);
            }
        } 
        else if (packet.valueType == TYPE_FLOAT) {
            ImGui::SetNextItemWidth(150);
            ImGui::InputFloat(label, floatMap[id], 0.01f, 1.0f, "%.3f");
            ImGui::SameLine();
            if (ImGui::Button("Apply")) {
                sendModifyPacket(id, packet.valueType);
            }
        } 
        else {
            // Integer / Default logic
            ImGui::SetNextItemWidth(150);
            ImGui::InputInt(label, intMap[id], 1, 10);
            ImGui::SameLine();
            if (ImGui::Button("Apply")) {
                sendModifyPacket(id, packet.valueType);
            }
        }

        ImGui::PopID();
        ImGui::Separator();
    }

    ImGui::Spacing();
    if (ImGui::Button("Refresh List", ImVec2(120, 0))) {
        refreshPackets();
    }

    ImGui::End();
}

void LiveDataPanel::stop() {
    liveDataMap.clear();
    intMap.clear();
    floatMap.clear();
}

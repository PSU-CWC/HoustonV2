#include "ControlPanel.h"
#include "../Util.h"
#include "../producer/SerialProducer.h"

void ControlPanel::start() {
    // Optionally refresh ports on start
    availablePorts = Util::getAvailablePorts();
}

void ControlPanel::render() {
    ImGui::Begin("Control Panel");

    // 1. Port Selection Section
    if (ImGui::Button("Refresh Ports") || availablePorts.empty()) {
        availablePorts = Util::getAvailablePorts();
        // Safety check to ensure index doesn't go out of bounds after refresh
        if (selectedPortIndex >= availablePorts.size()) {
            selectedPortIndex = 0;
        }
    }

    ImGui::SameLine();

    if (!availablePorts.empty()) {
        // Dropdown menu for ports
        if (ImGui::BeginCombo("Port", availablePorts[selectedPortIndex].c_str())) {
            for (int i = 0; i < availablePorts.size(); ++i) {
                const bool isSelected = (selectedPortIndex == i);
                if (ImGui::Selectable(availablePorts[i].c_str(), isSelected)) {
                    selectedPortIndex = i;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No serial ports detected.");
    }

    ImGui::Separator();

    // 2. Attach / Detach Logic
    if (isEnable) {
        // Show Red Detach button when connected
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Detach Device")) {
            if (dataProducer) {
                dataProducer->stop();
                delete dataProducer;
                dataProducer = nullptr;
            }
            for (auto &component : *pVector) {
                component->stop();
            }
            isEnable = false;
        }
        ImGui::PopStyleColor();
    } else {
        // Show Green Attach button when disconnected
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
        if (ImGui::Button("Attach Device")) {
            if (!availablePorts.empty()) {
                if (dataProducer == nullptr) {
                    dataProducer = new SerialProducer();
                }

                std::string selectedPort = availablePorts[selectedPortIndex];
                dynamic_cast<SerialProducer *>(dataProducer)->setPort(selectedPort);

                if (!dataProducer->start()) {
                    delete dataProducer;
                    dataProducer = nullptr;
                    // Error will show for one frame, consider a persistent status string
                } else {
                    for (auto &component : *pVector) {
                        component->start();
                    }
                    isEnable = true;
                }
            }
        }
        ImGui::PopStyleColor();
    }

    // 3. Status and Performance
    if (dataProducer != nullptr && dataProducer->status) {
        dataProducer->produce(dispatcher);
        dataProducer->send_data(dispatcher);
        
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected");
    }

    ImGui::Spacing();
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    
    ImGui::End();
}

void ControlPanel::stop() {
    // Clean up on app close
    if (isEnable) {
        if (dataProducer) dataProducer->stop();
        isEnable = false;
    }
}

ControlPanel::~ControlPanel() {
    if (dataProducer) {
        delete dataProducer;
    }
}
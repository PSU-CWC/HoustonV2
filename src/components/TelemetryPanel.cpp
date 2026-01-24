#include "TelemetryPanel.h"
#include "../consumer/TelemetryConsumer.h"
#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

void TelemetryPanel::start() {
    telemetryMap.clear();
    savingFile = false;
}

void TelemetryPanel::render() {
    // 1. Process Incoming Data from the Queue
    DataConsumer *consumer = dispatcher->getHandler(ID_Telemetry);
    if (consumer) {
        auto *queueData = dynamic_cast<TypedConsumer<std::tuple<std::string, std::string>> *>(consumer);
        while (queueData && !queueData->isEmpty()) {
            auto dataOpt = queueData->pop();
            if (dataOpt.has_value()) {
                auto [key, val] = dataOpt.value();
                telemetryMap[key] = val; 
            }
        }
    }

    // 2. Background File Logging (Runs every frame while savingFile is true)
    if (savingFile && file.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        file << ms.time_since_epoch().count() << ",";

        for (size_t i = 1; i < csvHeaders.size(); i++) {
            const std::string& key = csvHeaders[i];
            file << (telemetryMap.count(key) > 0 ? telemetryMap[key] : "0");
            if (i < csvHeaders.size() - 1) file << ",";
        }
        file << "\n";
    }

    // 3. UI Rendering
    ImGui::Begin("Telemetry");

    // Table View
    ImGui::Columns(2, "DataDisplay");
    ImGui::Separator();
    for (const auto &pair : telemetryMap) {
        ImGui::Text("%s", pair.first.c_str());
        ImGui::NextColumn();
        ImGui::Text("%s", pair.second.c_str());
        ImGui::NextColumn();
        ImGui::Separator();
    }
    ImGui::Columns(1); 

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::InputText("Filename", csvFileBuffer, IM_ARRAYSIZE(csvFileBuffer));

    if (savingFile) {
        // --- UI for STOPPING ---
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Stop Saving File")) {
            savingFile = false;
            file.close();
        }
        ImGui::PopStyleColor();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "REC: %s", csvFileBuffer);
    } else {
        // --- UI for STARTING ---
        if (ImGui::Button("Start Saving File")) {
            std::string filename = csvFileBuffer;
            if (filename.empty()) filename = "telemetry_data.csv";

            fs::path p(filename);

            // Force to Desktop if no path provided
            if (!p.has_parent_path()) {
                const char* homeDir = getenv("HOME");
                if (homeDir) {
                    p = fs::path(homeDir) / "Desktop" / p;
                }
            }

            csvHeaders.clear();
            csvHeaders.emplace_back("Time");

            file.open(p.string(), std::ios::out); 
            if (file.is_open()) {
                // Write Header Row
                file << "Time";
                for (const auto &pair : telemetryMap) {
                    csvHeaders.push_back(pair.first);
                    file << "," << pair.first;
                }
                file << "\n";
                savingFile = true;
                
                // Copy the resolved path back to the buffer so the user sees where it went
                strncpy(csvFileBuffer, p.string().c_str(), IM_ARRAYSIZE(csvFileBuffer) - 1);
            }
        }
    }

    // This MUST be the last line of the function and MUST be outside any if/else
    ImGui::End(); 
}

void TelemetryPanel::stop() {
    if (savingFile) {
        file.close();
        savingFile = false;
    }
    telemetryMap.clear();
    csvHeaders.clear();
}

TelemetryPanel::~TelemetryPanel() {
    stop();
}
#pragma once

#include <fstream>
#include <memory>
#include "../pch.h"

#include "Component.h"
#include "../Util.h"
#include "../Setting.h"

class TelemetryPanel : public Component {
private:
    std::map<std::string, std::string> telemetryMap;
    bool paused = false;
    bool initialized = false; // Fixed spelling
    float history = 10.0f;
    bool autoScale = true;
    
    std::map<std::string, std::unique_ptr<Util::ScrollingBuffer>> dataMap;
    std::map<std::string, bool> showMap; // Changed bool* to bool for safety
    
    char csvFileBuffer[256] = "data.csv";
    bool savingFile = false;
    std::vector<std::string> csvHeaders;
    std::ofstream file;

    int timer = 0;

public:
    TelemetryPanel(const char *name, Dispatcher *dispatcher) 
        : Component(name, dispatcher), initialized(true) { // Set to true here or in start()
    }

    ~TelemetryPanel() override;
    void start() override;
    void render() override;
    void stop() override;
    void graphData();
};
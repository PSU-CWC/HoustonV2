#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

// Platform-specific includes for port detection
#ifdef _WIN32
    #include <windows.h>
    #include <cstdio>
#else
    #include <glob.h>
#endif

namespace Util {

    // --- NEW: Cross-platform port detection ---
    inline std::vector<std::string> getAvailablePorts() {
        std::vector<std::string> ports;

    #ifdef _WIN32
        // Windows Logic: Scan COM1 through COM20
        char portName[20];
        for (int i = 1; i <= 20; i++) {
            std::sprintf(portName, "\\\\.\\COM%d", i);
            HANDLE hSerial = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            if (hSerial != INVALID_HANDLE_VALUE) {
                ports.push_back("COM" + std::to_string(i));
                CloseHandle(hSerial);
            }
        }
    #else
        // macOS Logic: Search for dial-out device files
        glob_t glob_result;
        // glob finds all files matching the pattern /dev/cu.*
        if (glob("/dev/cu.*", GLOB_TILDE, NULL, &glob_result) == 0) {
            for (unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
                ports.push_back(std::string(glob_result.gl_pathv[i]));
            }
            globfree(&glob_result);
        }
    #endif
        return ports;
    }

    // --- Your existing splitString, Endian helpers, and Structs ---
    inline auto splitString(std::string in, char sep) {
        std::vector<std::string> r;
        r.reserve(std::count(in.begin(), in.end(), sep) + 1);
        for (auto p = in.begin();; ++p) {
            auto q = p;
            p = std::find(p, in.end(), sep);
            r.emplace_back(q, p);
            if (p == in.end())
                return r;
        }
    }

    inline uint16_t toLittleEndian16(uint16_t value) {
        return ((value & 0xFF) << 8) | ((value & 0xFF00) >> 8);
    }

    inline uint32_t toLittleEndian32(uint32_t value) {
        return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) |
               ((value & 0xFF000000) >> 24);
    }

    // ... (ModifyPacket and ScrollingBuffer remain the same)
    struct ScrollingBuffer {
        int MaxSize;
        int Offset;
        ImVector<ImVec2> Data;

        ScrollingBuffer(int max_size = 5000) {
            MaxSize = max_size;
            Offset = 0;
            Data.reserve(MaxSize);
        }

        void AddPoint(float x, float y) {
            if (Data.size() < MaxSize)
                Data.push_back(ImVec2(x, y));
            else {
                Data[Offset] = ImVec2(x, y);
                Offset = (Offset + 1) % MaxSize;
            }
        }

        void Erase() {
            if (Data.size() > 0) {
                Data.shrink(0);
                Offset = 0;
            }
        }
    };
}
//
// Created by carter on 6/28/25.
//

#include "util.h"

#include <fstream>
#include <sstream>
#include <stdexcept>


std::string key_code_to_string(const int code) {
    auto it = keymap.find(code);
    if (it != keymap.end()) {
        return it->second;
    }
    return "UNKNOWN_KEY_" + std::to_string(code);
}

std::string find_keyboard_device() {
    std::ifstream devices_file("/proc/bus/input/devices");
    if (!devices_file.is_open()) {
        throw std::runtime_error(
            "Failed to open /proc/bus/input/devices. Are you root?");
    }

    std::string line;
    std::string current_device_handler;
    bool found_keyboard {false};

    while (std::getline(devices_file, line)) {
        if (line.empty()) {
            // Reset on blank lines which separate device entries
            current_device_handler = "";
            found_keyboard = false;
            continue;
        }

        std::stringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "H:") {
            // Handlers line
            std::string handlers;
            std::getline(ss, handlers); // Read the rest of the line
            std::stringstream handler_ss(handlers);
            std::string handler_part;
            while (handler_ss >> handler_part) {
                if (handler_part.rfind("event", 0) == 0) {
                    current_device_handler = "/dev/input/" + handler_part;
                }
            }
        } else if (key == "B:") {
            std::string pair;
            ss >> pair;
            std::stringstream pair_ss(pair);

            if (pair == "EV=120013") {
                found_keyboard = true;
            }
        }

        // If we found a keyboard and have a handler for it, we are done
        if (found_keyboard && !current_device_handler.empty()) {
            return current_device_handler;
        }
    }

    throw std::runtime_error(
        "Could not find a keyboard device. Check /proc/bus/input/devices.");
}

std::string map_to_shift(int code) {

    auto it = shift_map.find(code);
    if (it != shift_map.end()) {
        return it->second;
    }
    return key_code_to_string(code);
}

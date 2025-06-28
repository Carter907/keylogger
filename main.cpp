/******************************************************************************
 * Global Key Press Logger for Ubuntu/Linux
 *
 * This application reads raw input events directly from the kernel's
 * input subsystem (/dev/input). It is designed to demonstrate how to
 * capture keyboard events globally without needing a graphical display server
 * like X11 or Wayland.
 *
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
 *
 *
 * Compilation:
 * g++ -o key_logger key_logger.cpp
 *
 *
 * Usage:
 * You must run this application with root privileges to access /dev/input/.
 *
 * sudo ./key_logger
 *
 *
 * Dependencies:
 * Requires the standard C++ library and headers available on most Linux
 * systems, such as <iostream>, <string>, <fstream>, <stdexcept>, etc.
 * It also requires the Linux-specific header <linux/input.h>.
 *
 ******************************************************************************/

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/input.h>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

// Function to map key codes to their string representations.
// A more comprehensive map can be found in
// /usr/include/linux/input-event-codes.h
std::string key_code_to_string(int code) {
  static const std::map<int, std::string> keymap = {
      {KEY_RESERVED, "RESERVED"},
      {KEY_ESC, "ESC"},
      {KEY_1, "1"},
      {KEY_2, "2"},
      {KEY_3, "3"},
      {KEY_4, "4"},
      {KEY_5, "5"},
      {KEY_6, "6"},
      {KEY_7, "7"},
      {KEY_8, "8"},
      {KEY_9, "9"},
      {KEY_0, "0"},
      {KEY_MINUS, "-"},
      {KEY_EQUAL, "="},
      {KEY_BACKSPACE, "BACKSPACE"},
      {KEY_TAB, "TAB"},
      {KEY_Q, "Q"},
      {KEY_W, "W"},
      {KEY_E, "E"},
      {KEY_R, "R"},
      {KEY_T, "T"},
      {KEY_Y, "Y"},
      {KEY_U, "U"},
      {KEY_I, "I"},
      {KEY_O, "O"},
      {KEY_P, "P"},
      {KEY_LEFTBRACE, "["},
      {KEY_RIGHTBRACE, "]"},
      {KEY_ENTER, "ENTER"},
      {KEY_LEFTCTRL, "LEFTCTRL"},
      {KEY_A, "A"},
      {KEY_S, "S"},
      {KEY_D, "D"},
      {KEY_F, "F"},
      {KEY_G, "G"},
      {KEY_H, "H"},
      {KEY_J, "J"},
      {KEY_K, "K"},
      {KEY_L, "L"},
      {KEY_SEMICOLON, ";"},
      {KEY_APOSTROPHE, "'"},
      {KEY_GRAVE, "`"},
      {KEY_LEFTSHIFT, "LEFTSHIFT"},
      {KEY_BACKSLASH, "\\"},
      {KEY_Z, "Z"},
      {KEY_X, "X"},
      {KEY_C, "C"},
      {KEY_V, "V"},
      {KEY_B, "B"},
      {KEY_N, "N"},
      {KEY_M, "M"},
      {KEY_COMMA, ","},
      {KEY_DOT, "."},
      {KEY_SLASH, "/"},
      {KEY_RIGHTSHIFT, "RIGHTSHIFT"},
      {KEY_KPASTERISK, "KPASTERISK"},
      {KEY_LEFTALT, "LEFTALT"},
      {KEY_SPACE, "SPACE"},
      {KEY_CAPSLOCK, "CAPSLOCK"},
      {KEY_F1, "F1"},
      {KEY_F2, "F2"},
      {KEY_F3, "F3"},
      {KEY_F4, "F4"},
      {KEY_F5, "F5"},
      {KEY_F6, "F6"},
      {KEY_F7, "F7"},
      {KEY_F8, "F8"},
      {KEY_F9, "F9"},
      {KEY_F10, "F10"},
      {KEY_NUMLOCK, "NUMLOCK"},
      {KEY_SCROLLLOCK, "SCROLLLOCK"},
      {KEY_KP7, "KP7"},
      {KEY_KP8, "KP8"},
      {KEY_KP9, "KP9"},
      {KEY_KPMINUS, "KPMINUS"},
      {KEY_KP4, "KP4"},
      {KEY_KP5, "KP5"},
      {KEY_KP6, "KP6"},
      {KEY_KPPLUS, "KPPLUS"},
      {KEY_KP1, "KP1"},
      {KEY_KP2, "KP2"},
      {KEY_KP3, "KP3"},
      {KEY_KP0, "KP0"},
      {KEY_KPDOT, "KPDOT"},
      {KEY_UP, "UP"},
      {KEY_DOWN, "DOWN"},
      {KEY_LEFT, "LEFT"},
      {KEY_RIGHT, "RIGHT"},
      {KEY_PAGEUP, "PAGEUP"},
      {KEY_PAGEDOWN, "PAGEDOWN"},
      {KEY_HOME, "HOME"},
      {KEY_END, "END"},
      {KEY_INSERT, "INSERT"},
      {KEY_DELETE, "DELETE"}};

  auto it = keymap.find(code);
  if (it != keymap.end()) {
    return it->second;
  }
  return "UNKNOWN_KEY_" + std::to_string(code);
}

// Finds the keyboard device event file from /proc/bus/input/devices.
// It looks for a device with EV=120013, which is typical for a keyboard.
std::string find_keyboard_device() {
  std::ifstream devices_file("/proc/bus/input/devices");
  if (!devices_file.is_open()) {
    throw std::runtime_error(
        "Failed to open /proc/bus/input/devices. Are you root?");
  }

  std::string line;
  std::string current_device_handler;
  bool found_keyboard = false;

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
      // Bitmap line
      std::string pair;
      ss >> pair;

      std::cout << pair << std::endl;
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

int main() {
  int fd;
  struct input_event ev{};

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

    while (true) {
      // Read an input event from the device file
      ssize_t bytes = read(fd, &ev, sizeof(struct input_event));

      if (bytes == sizeof(struct input_event)) {
        // We are only interested in key press events
        // ev.type == EV_KEY means it's a key or button event
        // ev.value == 1 means the key was pressed down
        // ev.value == 0 means the key was released
        // ev.value == 2 means the key is being held down (autorepeat)
        if (ev.type == EV_KEY && ev.value == 1) {
          std::cout << "Key Pressed: " << key_code_to_string(ev.code)
                    << std::endl;
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

  close(fd);
  return 0;
}

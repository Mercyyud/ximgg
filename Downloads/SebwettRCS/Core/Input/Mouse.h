#pragma once
#include <Windows.h>

namespace Mouse {

// Open connection to Logitech GHUB HID driver (falls back to SendInput)
bool Open();

// Close the driver handle
void Close();

// Send relative mouse movement
void Move(int x, int y);

// Check if the Logitech driver is connected
bool IsConnected();

} // namespace Mouse

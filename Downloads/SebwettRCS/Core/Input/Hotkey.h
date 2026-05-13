#pragma once
#include <Windows.h>

namespace Hotkey {

// Returns the display name for a virtual key code
const char* GetKeyName(int vk);

// Returns true if any key is currently held (used for rebind capture)
bool IsAnyKeyDown();

} // namespace Hotkey

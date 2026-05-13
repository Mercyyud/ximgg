#include "Hotkey.h"
#include <unordered_map>

namespace Hotkey {

const char* GetKeyName(int vk) {
    static const std::unordered_map<int, const char*> names = {
        { VK_LBUTTON,  "L CLICK"     },
        { VK_RBUTTON,  "R CLICK"     },
        { VK_MBUTTON,  "MOUSE 3"     },
        { VK_XBUTTON1, "MOUSE 4"     },
        { VK_XBUTTON2, "MOUSE 5"     },
        { VK_INSERT,   "INSERT"      },
        { VK_DELETE,   "DELETE"      },
        { VK_HOME,     "HOME"        },
        { VK_END,      "END"         },
        { VK_PRIOR,    "PAGE UP"     },
        { VK_NEXT,     "PAGE DOWN"   },
        { VK_F1,  "F1"  }, { VK_F2,  "F2"  }, { VK_F3,  "F3"  },
        { VK_F4,  "F4"  }, { VK_F5,  "F5"  }, { VK_F6,  "F6"  },
        { VK_F7,  "F7"  }, { VK_F8,  "F8"  }, { VK_F9,  "F9"  },
        { VK_F10, "F10" }, { VK_F11, "F11" }, { VK_F12, "F12" },
        { VK_NUMPAD0, "NUM 0" }, { VK_NUMPAD1, "NUM 1" },
        { VK_NUMPAD2, "NUM 2" }, { VK_NUMPAD3, "NUM 3" },
        { VK_NUMPAD4, "NUM 4" }, { VK_NUMPAD5, "NUM 5" },
        { VK_NUMPAD6, "NUM 6" }, { VK_NUMPAD7, "NUM 7" },
        { VK_NUMPAD8, "NUM 8" }, { VK_NUMPAD9, "NUM 9" },
        { VK_PAUSE,    "PAUSE"       },
        { VK_SCROLL,   "SCROLL LOCK" },
        { VK_CAPITAL,  "CAPS LOCK"   },
        { VK_TAB,      "TAB"         },
        { VK_ESCAPE,   "ESCAPE"      },
        { VK_SPACE,    "SPACE"       },
        { VK_BACK,     "BACKSPACE"   },
        { VK_RETURN,   "ENTER"       },
        { VK_LCONTROL, "L-CTRL"      },
        { VK_RCONTROL, "R-CTRL"      },
        { VK_LSHIFT,   "L-SHIFT"     },
        { VK_RSHIFT,   "R-SHIFT"     },
        { VK_LMENU,    "L-ALT"       },
        { VK_RMENU,    "R-ALT"       },
        { VK_OEM_3,    "TILDE"       },
    };

    auto it = names.find(vk);
    if (it != names.end()) return it->second;

    static char buf[16];
    if (vk >= 'A' && vk <= 'Z') { buf[0] = (char)vk; buf[1] = '\0'; return buf; }
    if (vk >= '0' && vk <= '9') { buf[0] = (char)vk; buf[1] = '\0'; return buf; }
    sprintf_s(buf, "0x%02X", vk);
    return buf;
}

bool IsAnyKeyDown() {
    for (int i = 0x08; i <= 0xFE; ++i)
        if (GetAsyncKeyState(i) & 0x8000) return true;
    return false;
}

} // namespace Hotkey

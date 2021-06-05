#pragma once
#include <bitset>
#include <queue>
#include "keybinds.h"

class Settings {
public:

    KeyBinds::Config KeyBinds = {
        VK_DELETE
    };

    bool ImGuiInitialized = false;
    bool ShowMenu = false;

    void Load();
    void Save();
};

extern Settings State;
#pragma once

#include "SimpleIni.h"


namespace Settings
{

    inline const char* setting_path{"Data/SKSE/Plugins/ImmersiveWeaponSwitch.ini"};

    void LoadSettings();
    void SaveSettings();
    void ResetSettings();

    inline bool Mod_Active = true;

    inline bool NPC_Switch = true;
    inline bool PC_Switch = true;

    inline std::vector<std::string> blockedRaceSubstrings;
};

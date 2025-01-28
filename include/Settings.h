#pragma once

#include "SimpleIni.h"

namespace Settings {

    constexpr auto setting_path{"Data/SKSE/Plugins/ImmersiveWeaponSwitch.ini"};

    void LoadSettings();
    void SaveSettings();

    inline bool NPC_Drop_Weapons = true;
    inline bool PC_Drop_Weapons = false;
    inline bool Hold_To_Drop = true;
    inline float NPC_Health_Drop = 10.0;
    inline float PC_Health_Drop = 10.0;
}
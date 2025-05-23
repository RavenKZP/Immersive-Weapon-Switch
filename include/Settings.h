#pragma once

#include "SimpleIni.h"


namespace Settings
{

    inline const char* setting_path{"Data/SKSE/Plugins/ImmersiveWeaponSwitch.ini"};

    void LoadSettings();
    void SaveSettings();
    void ResetSettings();

    inline bool Mod_Active = true;

    inline bool NPC_Drop_Weapons = true;
    inline bool PC_Drop_Weapons = false;
    inline bool Followers_Drop_Weapons = false;
    inline float NPC_Health_Drop = 10.0f;
    inline float PC_Health_Drop = 10.0f;
    inline float Followers_Health_Drop = 10.0f;

    inline bool Hold_To_Drop = true;
    inline bool Hold_To_Unarmed = true;
    inline float Held_Duration = 0.7f;

    inline bool NPC_Switch = true;
    inline bool PC_Switch = true;
};

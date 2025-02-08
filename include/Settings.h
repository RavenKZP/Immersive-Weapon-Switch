#pragma once

#include "SimpleIni.h"


class Settings : public clib_util::singleton::ISingleton<Settings>
{
public:

    const char* setting_path{"Data/SKSE/Plugins/ImmersiveWeaponSwitch.ini"};

    void LoadSettings();
    void SaveSettings();
    void ResetSettings();

    bool Mod_Active = true;

    bool NPC_Drop_Weapons = true;
    bool PC_Drop_Weapons = false;
    bool Hold_To_Drop = true;
    bool Hold_To_Unarmed = true;
    float NPC_Health_Drop = 10.0f;
    float PC_Health_Drop = 10.0f;
    float Held_Duration = 0.7f;

    float PosX = 0.9f;
    float PosY = 0.9f;

private:
	static void SerializeINI(const wchar_t* a_path, const std::function<void(CSimpleIniA&)>& a_func, bool a_generate = false);
	static void SerializeINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, const std::function<void(CSimpleIniA&)>& a_func);

	const wchar_t* defaultDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks.ini" };
	const wchar_t* userDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks_Custom.ini" };
};

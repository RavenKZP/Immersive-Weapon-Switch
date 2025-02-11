
#include "Settings.h"

namespace Settings {

    void LoadSettings() {
        CSimpleIniW ini;
        ini.SetUnicode();

        if (!std::filesystem::exists(setting_path)) {
            logger::info("No {} file found, creating new with default values", setting_path);
            SaveSettings();
        } else {
            ini.LoadFile(setting_path);

            NPC_Drop_Weapons = ini.GetBoolValue(L"Settings", L"NPC_Drop_Weapons");
            PC_Drop_Weapons = ini.GetBoolValue(L"Settings", L"PC_Drop_Weapons");
            Hold_To_Drop = ini.GetBoolValue(L"Settings", L"Hold_To_Drop");
            Hold_To_Unarmed = ini.GetBoolValue(L"Settings", L"Hold_To_Unarmed");
            NPC_Health_Drop = ini.GetDoubleValue(L"Settings", L"NPC_Health_Drop");
            PC_Health_Drop = ini.GetDoubleValue(L"Settings", L"NPC_Health_Drop");
            Held_Duration = ini.GetDoubleValue(L"Settings", L"Held_Duration");

            logger::info("Settings Loaded");
        }
    }

    void Settings::SaveSettings() {
        CSimpleIniW ini;
        ini.SetUnicode();
        ini.LoadFile(setting_path);
        ini.Reset();
        ini.SetBoolValue(L"Settings", L"NPC_Drop_Weapons", NPC_Drop_Weapons);
        ini.SetDoubleValue(L"Settings", L"NPC_Health_Drop", NPC_Health_Drop);

        ini.SetBoolValue(L"Settings", L"PC_Drop_Weapons", PC_Drop_Weapons);
        ini.SetDoubleValue(L"Settings", L"PC_Health_Drop", PC_Health_Drop);

        ini.SetBoolValue(L"Settings", L"Hold_To_Drop", Hold_To_Drop);
        ini.SetBoolValue(L"Settings", L"Hold_To_Unarmed", Hold_To_Unarmed);
        ini.SetDoubleValue(L"Settings", L"Held_Duration", Held_Duration);

        ini.SaveFile(setting_path);

        logger::info("Settings Saved");
    }

    void Settings::ResetSettings() {
        NPC_Drop_Weapons = true;
        PC_Drop_Weapons = false;
        Hold_To_Drop = true;
        Hold_To_Unarmed = true;
        NPC_Health_Drop = 10.0f;
        PC_Health_Drop = 10.0f;
        Held_Duration = 0.7f;

        logger::info("Settings Reseted");
    }

}
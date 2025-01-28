#include "Settings.h"	

namespace Settings {

    void LoadSettings() {
        CSimpleIniW ini;
        ini.SetUnicode();

        if (!std::filesystem::exists(setting_path)) {
            logger::info("No {} file found, creating new with default values", setting_path);
            SaveSettings();
        } else {
            logger::info("Reading settings from file");
            ini.LoadFile(setting_path);

            NPC_Drop_Weapons = ini.GetBoolValue(L"Settings", L"NPC_Drop_Weapons");
            PC_Drop_Weapons = ini.GetBoolValue(L"Settings", L"PC_Drop_Weapons");
            Hold_To_Drop = ini.GetBoolValue(L"Settings", L"Hold_To_Drop");
            NPC_Health_Drop = ini.GetDoubleValue(L"Settings", L"NPC_Health_Drop");
            PC_Health_Drop = ini.GetDoubleValue(L"Settings", L"NPC_Health_Drop");

            logger::info("Settings loaded");
        }
    }

    void SaveSettings() {
        CSimpleIniW ini;
        ini.SetUnicode();
        ini.LoadFile(setting_path);
        ini.Reset();
        ini.SetBoolValue(L"Settings", L"NPC_Drop_Weapons", NPC_Drop_Weapons);
        ini.SetBoolValue(L"Settings", L"PC_Drop_Weapons", PC_Drop_Weapons);
        ini.SetBoolValue(L"Settings", L"Hold_To_Drop", Hold_To_Drop);
        ini.SetDoubleValue(L"Settings", L"NPC_Health_Drop", NPC_Health_Drop);
        ini.SetDoubleValue(L"Settings", L"PC_Health_Drop", PC_Health_Drop);
        ini.SaveFile(setting_path);
    }
}
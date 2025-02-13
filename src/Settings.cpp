
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

            Mod_Active = ini.GetBoolValue(L"Settings", L"Mod_Active");
            PC_Switch = ini.GetBoolValue(L"Settings", L"PC_Switch");
            NPC_Switch = ini.GetBoolValue(L"Settings", L"NPC_Switch");

            NPC_Drop_Weapons = ini.GetBoolValue(L"Settings", L"NPC_Drop_Weapons");
            PC_Drop_Weapons = ini.GetBoolValue(L"Settings", L"PC_Drop_Weapons");
            Followers_Drop_Weapons = ini.GetBoolValue(L"Settings", L"Followers_Drop_Weapons");

            NPC_Health_Drop = ini.GetDoubleValue(L"Settings", L"NPC_Health_Drop");
            PC_Health_Drop = ini.GetDoubleValue(L"Settings", L"NPC_Health_Drop");
            Followers_Health_Drop = ini.GetDoubleValue(L"Settings", L"Followers_Health_Drop");


            Hold_To_Drop = ini.GetBoolValue(L"Settings", L"Hold_To_Drop");
            Hold_To_Unarmed = ini.GetBoolValue(L"Settings", L"Hold_To_Unarmed");
            Held_Duration = ini.GetDoubleValue(L"Settings", L"Held_Duration");


            logger::info("Settings Loaded");
        }
    }

    void Settings::SaveSettings() {
        CSimpleIniW ini;
        ini.SetUnicode();
        ini.LoadFile(setting_path);
        ini.Reset();
        ini.SetBoolValue(L"Settings", L"Mod_Active", Mod_Active);
        ini.SetBoolValue(L"Settings", L"PC_Switch", PC_Switch);
        ini.SetBoolValue(L"Settings", L"NPC_Switch", NPC_Switch);

        ini.SetBoolValue(L"Settings", L"NPC_Drop_Weapons", NPC_Drop_Weapons);
        ini.SetBoolValue(L"Settings", L"PC_Drop_Weapons", PC_Drop_Weapons);
        ini.SetBoolValue(L"Settings", L"Followers_Drop_Weapons", Followers_Drop_Weapons);

        ini.SetDoubleValue(L"Settings", L"NPC_Health_Drop", NPC_Health_Drop);
        ini.SetDoubleValue(L"Settings", L"PC_Health_Drop", PC_Health_Drop);
        ini.SetDoubleValue(L"Settings", L"Followers_Health_Drop", Followers_Health_Drop);

        ini.SetBoolValue(L"Settings", L"Hold_To_Drop", Hold_To_Drop);
        ini.SetBoolValue(L"Settings", L"Hold_To_Unarmed", Hold_To_Unarmed);
        ini.SetDoubleValue(L"Settings", L"Held_Duration", Held_Duration);


        ini.SaveFile(setting_path);

        logger::info("Settings Saved");
    }

    void Settings::ResetSettings() {
        Mod_Active = true;

        NPC_Drop_Weapons = true;
        PC_Drop_Weapons = false;
        Followers_Drop_Weapons = false;
        NPC_Health_Drop = 10.0f;
        PC_Health_Drop = 10.0f;
        Followers_Health_Drop = 10.0f;

        Hold_To_Drop = true;
        Hold_To_Unarmed = true;
        Held_Duration = 0.7f;

        NPC_Switch = true;
        PC_Switch = true;

        logger::info("Settings Reseted");
    }

}
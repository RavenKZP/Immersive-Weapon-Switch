
#include "Settings.h"
#include "Renderer.h"

void Settings::SerializeINI(const wchar_t* a_path, const std::function<void(CSimpleIniA&)>& a_func, const bool a_generate)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (const auto rc = ini.LoadFile(a_path); !a_generate && rc < SI_OK) {
		return;
	}

	a_func(ini);

	(void)ini.SaveFile(a_path);
}

void Settings::SerializeINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, const std::function<void(CSimpleIniA&)>& a_func)
{
	SerializeINI(a_defaultPath, a_func);
	SerializeINI(a_userPath, a_func);
}

void Settings::LoadSettings()
{
	SerializeINI(defaultDisplayTweaksPath, userDisplayTweaksPath, [](auto& ini) {
		ImGui::Renderer::LoadSettings(ini);  // display tweaks scaling
	});

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

        PosX = ini.GetDoubleValue(L"Interface", L"Position_X");
        PosY = ini.GetDoubleValue(L"Interface", L"Position_Y");

        logger::info("Settings loaded");
    }
}

void Settings::SaveSettings() {
    CSimpleIniW ini;
    ini.SetUnicode();
    ini.LoadFile(setting_path);
    ini.Reset();
    ini.SetBoolValue(L"Settings", L"NPC_Drop_Weapons", NPC_Drop_Weapons);
    ini.SetBoolValue(L"Settings", L"PC_Drop_Weapons", PC_Drop_Weapons);
    ini.SetBoolValue(L"Settings", L"Hold_To_Drop", Hold_To_Drop);
    ini.SetBoolValue(L"Settings", L"Hold_To_Unarmed", Hold_To_Unarmed);
    ini.SetDoubleValue(L"Settings", L"NPC_Health_Drop", NPC_Health_Drop);
    ini.SetDoubleValue(L"Settings", L"PC_Health_Drop", PC_Health_Drop);
    ini.SetDoubleValue(L"Settings", L"Held_Duration", Held_Duration);

    ini.SetDoubleValue(L"Interface", L"Position_X", PosX);
    ini.SetDoubleValue(L"Interface", L"Position_Y", PosY);
    ini.SaveFile(setting_path);
}

void Settings::ResetSettings() {
    NPC_Drop_Weapons = true;
    PC_Drop_Weapons = false;
    Hold_To_Drop = true;
    Hold_To_Unarmed = true;
    NPC_Health_Drop = 10.0f;
    PC_Health_Drop = 10.0f;
    Held_Duration = 0.7f;

    PosX = 0.9f;
    PosY = 0.9f;
}



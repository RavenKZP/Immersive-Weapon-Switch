
#include "Settings.h"

#include <codecvt>

namespace Settings {

    void LoadSettings() {
        CSimpleIniW ini;
        ini.SetUnicode();

        if (!std::filesystem::exists(setting_path)) {
            logger::warn("[LoadSettings] No {} file found, creating new with default values", setting_path);
            SaveSettings();
        } else {
            ini.LoadFile(setting_path);

            Mod_Active = ini.GetBoolValue(L"Settings", L"Mod_Active");
            PC_Switch = ini.GetBoolValue(L"Settings", L"PC_Switch");
            NPC_Switch = ini.GetBoolValue(L"Settings", L"NPC_Switch");

            const wchar_t* keywordList = ini.GetValue(L"RaceBlocklist", L"BlockedKeywords", L"");
            std::wstring str = keywordList ? keywordList : L"";
            std::wstringstream ss(str);
            std::wstring token;
            while (std::getline(ss, token, L',')) {
                token.erase(0, token.find_first_not_of(L" \t"));
                token.erase(token.find_last_not_of(L" \t") + 1);
                if (!token.empty()) {
                    using convert_type = std::codecvt_utf8<wchar_t>;
                    std::wstring_convert<convert_type, wchar_t> converter;
                    std::string converted_str = converter.to_bytes(token);
                    blockedRaceSubstrings.emplace_back(converted_str);
                }
            }

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

        ini.SaveFile(setting_path);

        logger::info("[SaveSettings] Settings Saved");
    }

    void Settings::ResetSettings() {
        Mod_Active = true;

        NPC_Switch = true;
        PC_Switch = true;

        logger::info("[ResetSettings] Settings Reseted");
    }

}
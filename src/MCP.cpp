#include "MCP.h"
#include "Utils.h"

#include "SKSEMCP/SKSEMenuFramework.hpp"

namespace MCP {

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::warn("SKSE Menu Framework is not installed. Cannot register menu.");
            return;
        }
        SKSEMenuFramework::SetSection("Immersive Weapon Switch");
        SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);
        SKSEMenuFramework::AddSectionItem("Interface", RenderInterface);

        logger::info("SKSE Menu Framework registered.");
    }

    void __stdcall RenderSettings() {
        auto Settings = Settings::GetSingleton();
        if (ImGuiMCP::Button("Save Settings")) {
            Settings->SaveSettings();
        }
        if (ImGuiMCP::Button("Reset Settings")) {
            Settings->ResetSettings();
        }
        ImGuiMCP::Checkbox("Mod Active", &Settings->Mod_Active);

        ImGuiMCP::Checkbox("Hold 'R' To Drop", &Settings->Hold_To_Drop);
        ImGuiMCP::Checkbox("Hold 'R' To Unarmed", &Settings->Hold_To_Unarmed);
        ImGuiMCP::SliderFloat("Held Duration To Drop (sec)", &Settings->Held_Duration, 0, 10, "%.3f");

        ImGuiMCP::Checkbox("NPC Drop Weapons On Change", &Settings->NPC_Drop_Weapons);
        ImGuiMCP::Checkbox("PC Drop Weapons On Change", &Settings->PC_Drop_Weapons);

        ImGuiMCP::SliderFloat("NPC Health % to Drop", &Settings->NPC_Health_Drop, 0, 100, "%.0f");
        ImGuiMCP::SliderFloat("PC Health % to Drop", &Settings->PC_Health_Drop, 0, 100, "%.0f");
    }

    void __stdcall RenderInterface() {
        auto Settings = Settings::GetSingleton();
        if (ImGuiMCP::Button("Save Settings")) {
            Settings->SaveSettings();
        }
        if (ImGuiMCP::Button("Reset Settings")) {
            Settings->ResetSettings();
        }

        if (ImGuiMCP::Button("Show/Hide Switch Icons")) {
            if (Utils::ShowSwitchIcons) {
                Utils::ShowSwitchIcons = false;
            } else {
                Utils::ShowSwitchIcons = true;
            }
        }

        ImGuiMCP::SliderFloat("UI Position X", &Settings->PosX, 0, 1, "%.3f");
        ImGuiMCP::SliderFloat("UI Position Y", &Settings->PosY, 0, 1, "%.3f");

    }

}

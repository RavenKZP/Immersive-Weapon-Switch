#include "MCP.h"
#include "Utils.h"


namespace MCP {

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::warn("SKSE Menu Framework is not installed. Cannot register menu.");
            return;
        }
        SKSEMenuFramework::SetSection("Immersive Weapon Switch");
        SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);

        logger::info("SKSE Menu Framework registered.");
    }
    void __stdcall MCP::RenderSettings() {
        if (ImGui::Button("Save Settings")) {
            Settings::SaveSettings();
        }

        ImGui::Checkbox("Mod Active", &Utils::Mod_Active);

        ImGui::Checkbox("NPC Can Drop Weapons on change", &Settings::NPC_Drop_Weapons);
        ImGui::Checkbox("PC Can Drop Weapons on change", &Settings::PC_Drop_Weapons);
        ImGui::Checkbox("Hold 'R' To Drop", &Settings::Hold_To_Drop);

        ImGui::SliderFloat("Max NPC Health to Drop", &Settings::NPC_Health_Drop, 0, 100, "%.0f");
        ImGui::SliderFloat("Max PC Health to Drop", &Settings::PC_Health_Drop, 0, 100, "%.0f");

    };
}
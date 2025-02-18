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

        logger::info("SKSE Menu Framework registered.");
    }

    void __stdcall RenderSettings() {
        if (ImGui::Button("Save Settings")) {
            Settings::SaveSettings();
        }
        if (ImGui::Button("Reset Settings")) {
            Settings::ResetSettings();
        }
        ImGui::Text("Enable/Disable Mod or it's parts");
        ImGui::Checkbox("Mod Active", &Settings::Mod_Active);
        ImGui::Checkbox("NPC Active", &Settings::NPC_Switch);
        ImGui::Checkbox("PC Active", &Settings::PC_Switch);

        /*
        ImGui::Text("Player Drop Weapon options");
        ImGui::Checkbox("Hold 'R' To Drop", &Settings::Hold_To_Drop);
        ImGui::Checkbox("Hold 'R' To Unarmed", &Settings::Hold_To_Unarmed);
        ImGui::SliderFloat("Held Duration To Drop (sec)", &Settings::Held_Duration, 0, 10, "%.3f");
        ImGui::Checkbox("PC Drop Weapons On Change", &Settings::PC_Drop_Weapons);
        ImGui::SliderFloat("PC Health % to Drop", &Settings::PC_Health_Drop, 0, 100, "%.0f");

        ImGui::Text("NPC Drop Weapon options");
        ImGui::Checkbox("NPC Drop Weapons On Change", &Settings::NPC_Drop_Weapons);
        ImGui::SliderFloat("NPC Health % to Drop", &Settings::NPC_Health_Drop, 0, 100, "%.0f");
        ImGui::Checkbox("Followers Drop Weapons On Change", &Settings::Followers_Drop_Weapons);
        ImGui::SliderFloat("Followers Health % to Drop", &Settings::Followers_Health_Drop, 0, 100, "%.0f");
        */
    }

}

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

#ifndef NDEBUG
    SKSEMenuFramework::AddSectionItem("Log", RenderLog);
#endif

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

        if (!Settings::Mod_Active) {
            Utils::ClearAllEvents();
        }
    }

    void __stdcall MCP::RenderLog() {
        ImGui::Checkbox("Trace", &MCPLog::log_trace);
        ImGui::SameLine();
        ImGui::Checkbox("Info", &MCPLog::log_info);
        ImGui::SameLine();
        ImGui::Checkbox("Warning", &MCPLog::log_warning);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &MCPLog::log_error);
        ImGui::InputText("Custom Filter", MCPLog::custom, 255);

        // if"Generate Log" button is pressed, read the log file
        if (ImGui::Button("Generate Log")) {
            logLines = MCPLog::ReadLogFile();
        }

        // Display each line in a new ImGui::Text() element
        for (const auto& line : logLines) {
            if (line.find("trace") != std::string::npos && !MCPLog::log_trace) continue;
            if (line.find("info") != std::string::npos && !MCPLog::log_info) continue;
            if (line.find("warning") != std::string::npos && !MCPLog::log_warning) continue;
            if (line.find("error") != std::string::npos && !MCPLog::log_error) continue;
            if (line.find(MCPLog::custom) == std::string::npos && MCPLog::custom != "") continue;
            ImGui::Text(line.c_str());
        }
    }
}

namespace MCPLog {
    std::filesystem::path GetLogPath() {
        const auto logsFolder = SKSE::log::log_directory();
        if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
        auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
        auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
        return logFilePath;
    }

    std::vector<std::string> ReadLogFile() {
        std::vector<std::string> logLines;

        // Open the log file
        std::ifstream file(GetLogPath().c_str());
        if (!file.is_open()) {
            // Handle error
            return logLines;
        }

        // Read and store each line from the file
        std::string line;
        while (std::getline(file, line)) {
            logLines.push_back(line);
        }

        file.close();

        return logLines;
    }

}

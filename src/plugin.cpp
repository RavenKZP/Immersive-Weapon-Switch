
#include "Hooks.h"
#include "Logger.h"
#include "MCP.h"
#include "Renderer.h"
#include "Settings.h"
#include "Styles.h"
#include "Utils.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        Settings::GetSingleton()->LoadSettings();
        Utils::InitGlobals();
        MCP::Register();
    }
    if (message->type == SKSE::MessagingInterface::kNewGame ||
        message->type == SKSE::MessagingInterface::kPreLoadGame) {
        Utils::ClearQueue();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    Hooks::Install();
    ImGui::Styles::GetSingleton()->RefreshStyle();
    ImGui::Renderer::Install();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}

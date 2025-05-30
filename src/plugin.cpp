
#include "logger.h"
#include "Utils.h"
#include "Hooks.h"
#include "MCP.h"
#include "Settings.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        Hooks::InstallReadOnly();
        Settings::LoadSettings();
        Utils::InitGlobals();
        MCP::Register();
    }
    if (message->type == SKSE::MessagingInterface::kNewGame ||
        message->type == SKSE::MessagingInterface::kPreLoadGame) {
        Utils::ClearAllEvents();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    logger::info("Game version: {}", skse->RuntimeVersion().string());
    Hooks::Install();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}

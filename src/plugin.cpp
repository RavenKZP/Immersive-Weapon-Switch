
#include "Events.h"
#include "Hooks.h"
#include "Logger.h"
#include "MCP.h"
#include "Settings.h"
#include "Utils.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        Settings::LoadSettings();
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
    logger::info("Game version: {}", skse->RuntimeVersion().string());
    Hooks::Install();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}

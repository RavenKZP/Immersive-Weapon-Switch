#include "Hooks.h"
#include "MCP.h"
#include "Settings.h"
#include "Utils.h"
#include "logger.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        Settings::LoadSettings();
        MCP::Register();
        Utils::InitGlobals();
    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPreLoadGame) {
        Utils::ClearQueue();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Game version: {}", skse->RuntimeVersion().string());
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    Hooks::Install();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}

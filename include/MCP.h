#pragma once

#include "Settings.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"

constexpr ImGuiTableFlags table_flags =
    ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

namespace MCP {

    void Register();
    void __stdcall RenderSettings();

};
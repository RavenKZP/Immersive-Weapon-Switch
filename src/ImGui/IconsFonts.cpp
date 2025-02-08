#include "IconsFonts.h"
#include "IconsFontAwesome6.h"
#include "Input.h"
#include "Renderer.h"
#include "Util.h"
#include "imgui_internal.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>

#include "MCP.h"


namespace IconFont
{
	IconTexture::IconTexture(const std::wstring_view a_iconName) :
		ImGui::Texture(LR"(Data/Interface/ImGuiIcons/Icons/)", a_iconName)
	{}

	bool IconTexture::Load(const bool a_resizeToScreenRes)
	{
		const bool result = ImGui::Texture::Load(a_resizeToScreenRes);

		if (result) {
			// store original size
			imageSize = size;
			// don't need this
			if (image) {
				image.reset();
			}
		}

		return result;
	}

	void IconTexture::Resize(const float a_scale)
	{
		const auto scale = a_scale / 1080;  // standard window height
		size = imageSize * (scale * RE::BSGraphics::Renderer::GetScreenSize().height);
	}

	void Manager::LoadIcons()
	{
		unknownKey.Load();

		upKey.Load();
		downKey.Load();
		leftKey.Load();
		rightKey.Load();

		std::ranges::for_each(keyboard, [](auto& IconTexture) {
			IconTexture.second.Load();
		});
		std::ranges::for_each(gamePad, [](auto& IconTexture) {
			auto& [xbox, ps4] = IconTexture.second;
			xbox.Load();
			ps4.Load();
		});
		std::ranges::for_each(mouse, [](auto& IconTexture) {
			IconTexture.second.Load();
		});

		stepperLeft.Load();
		stepperRight.Load();
		checkbox.Load();
		checkboxFilled.Load();
	}

	void Manager::ReloadFonts()
	{
		auto& io = ImGui::GetIO();
		io.Fonts->Clear();

		ImVector<ImWchar> ranges;

		ImFontGlyphRangesBuilder builder;
		builder.AddText(RE::BSScaleformManager::GetSingleton()->validNameChars.c_str());
        builder.AddChar(0x20);		// Space
        builder.AddChar(0xf363);	// ICON_FA_REPEAT
        builder.AddChar(0xf132);	// ICON_FA_SHIELD
        builder.AddChar(0xf6de);	// ICON_FA_HAND_FIST
        builder.AddChar(0xe05d);    // ICON_FA_HAND_SPARKLES
        builder.AddChar(0xf06d);    // ICON_FA_FIRE
        builder.AddChar(0xf71c);
        
		builder.BuildRanges(&ranges);

		const auto resolutionScale = ImGui::Renderer::GetResolutionScale();
        logger::trace("resolutionScale {}", resolutionScale);
		auto a_fontsize = resolutionScale * 40.0f;
		auto a_iconsize = a_fontsize * 0.9f;
		auto a_largefontsize = a_fontsize * 1.2f;
		auto a_largeiconsize = a_largefontsize * 0.9f;

		io.FontDefault = LoadFontIconSet(a_fontsize, a_iconsize, ranges);
		largeFont = LoadFontIconSet(a_largefontsize, a_largeiconsize, ranges);

		io.Fonts->Build();

		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();
	}

	ImFont* Manager::LoadFontIconSet(const float a_fontSize, const float a_iconSize, const ImVector<ImWchar>& a_ranges) const
	{
		const auto& io = ImGui::GetIO();
		const auto font = io.Fonts->AddFontFromFileTTF(fontName.c_str(), a_fontSize, nullptr, a_ranges.Data);
		if (!font) {
			logger::error("Failed to load font: {}", fontName);
			return nullptr;
		}
		ImFontConfig icon_config;
		icon_config.MergeMode = true;
		icon_config.PixelSnapH = true;
		icon_config.OversampleH = icon_config.OversampleV = 1;

		io.Fonts->AddFontFromFileTTF(R"(Data\Interface\ImGuiIcons\Fonts\)" FONT_ICON_FILE_NAME_FAS, a_iconSize, &icon_config, a_ranges.Data);

		return font;
	}

	ImFont* Manager::GetLargeFont() const
	{
		return largeFont;
	}

	const IconTexture* Manager::GetStepperLeft() const
	{
		return &stepperLeft;
	}
	const IconTexture* Manager::GetStepperRight() const
	{
		return &stepperRight;
	}

	const IconTexture* Manager::GetCheckbox() const
	{
		return &checkbox;
	}
	const IconTexture* Manager::GetCheckboxFilled() const
	{
		return &checkboxFilled;
	}

	const IconTexture* Manager::GetIcon(const std::uint32_t key)
	{
		switch (key) {
		case KEY::kUp:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_UP:
			return &upKey;
		case KEY::kDown:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_DOWN:
			return &downKey;
		case KEY::kLeft:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_LEFT:
			return &leftKey;
		case KEY::kRight:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_RIGHT:
			return &rightKey;
		default:
			{
				if (const auto inputDevice = MANAGER(Input)->GetInputDevice(); inputDevice == Input::DEVICE::kKeyboard || inputDevice == Input::DEVICE::kMouse) {
					if (key >= SKSE::InputMap::kMacro_MouseButtonOffset) {
						if (const auto it = mouse.find(key); it != mouse.end()) {
							return &it->second;
						}
					} else if (const auto it = keyboard.find(static_cast<KEY>(key)); it != keyboard.end()) {
						return &it->second;
					}
				} else {
					if (const auto it = gamePad.find(key); it != gamePad.end()) {
						return GetGamePadIcon(it->second);
					}
				}
				return &unknownKey;
			}
		}
	}

	std::set<const IconTexture*> Manager::GetIcons(const std::set<std::uint32_t>& keys)
	{
		std::set<const IconTexture*> icons{};
		if (keys.empty()) {
			icons.insert(&unknownKey);
		} else {
			for (auto& key : keys) {
				icons.insert(GetIcon(key));
			}
		}
		return icons;
	}

	const IconTexture* Manager::GetGamePadIcon(const GamepadIcon& a_icons) const
	{
		switch (buttonScheme) {
		case BUTTON_SCHEME::kAutoDetect:
			return MANAGER(Input)->GetInputDevice() == Input::DEVICE::kGamepadOrbis ? &a_icons.ps4 : &a_icons.xbox;
		case BUTTON_SCHEME::kXbox:
			return &a_icons.xbox;
		case BUTTON_SCHEME::kPS4:
			return &a_icons.ps4;
		default:
			return &a_icons.xbox;
		}
	}
}

ImVec2 ImGui::ButtonIcon(const std::uint32_t a_key)
{
	const auto IconTexture = MANAGER(IconFont)->GetIcon(a_key);
	return ButtonIcon(IconTexture, false);
}

ImVec2 ImGui::ButtonIcon(const IconFont::IconTexture* a_texture, const bool a_centerIcon)
{
	ImVec2 a_size = {1, 1};
	if (a_centerIcon) {
		const float height = ImGui::GetWindowSize().y;
		ImGui::SetCursorPosY((height - a_size.y) / 2);
	}
	ImGui::Image(reinterpret_cast<ImTextureID>(a_texture->srView.Get()), a_size);

	return a_size;
}


void ImGui::ButtonIcon(const std::set<const IconFont::IconTexture*>& a_texture, const bool a_centerIcon)
{
	BeginGroup();
	for (auto& IconTexture : a_texture) {
		auto       pos = ImGui::GetCursorPos();
		const auto size = ImGui::ButtonIcon(IconTexture, a_centerIcon);
		ImGui::SetCursorPos({ pos.x + size.x, pos.y });
	}
	EndGroup();
}

void ImGui::ButtonIconWithLabel(const char* a_text, const IconFont::IconTexture* a_texture, const bool a_centerIcon)
{
	ImGui::ButtonIcon(a_texture, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}

void ImGui::ButtonIconWithLabel(const char* a_text, const std::set<const IconFont::IconTexture*>& a_texture, const bool a_centerIcon)
{
	ImGui::ButtonIcon(a_texture, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}

void ImGui::DrawCircle(ImDrawList* drawList, const ImVec2 a_center, const float a_radius, const float progress)
{
    constexpr int numSegments = 64;
    constexpr float startAngle = -IM_PI / 2; // Start at the top
    const float endAngle = startAngle + progress * 2.0f * IM_PI;
	drawList->PathArcTo(a_center, a_radius, startAngle, endAngle, numSegments);
    drawList->PathStroke(IM_COL32(255, 255, 255, 255), false, 4.0f);
}




#include "Hooks.h"


namespace Hooks {
    void ReadyWeaponHandlerHook::InstallHook()
    {
        REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_ReadyWeaponHandler[0]);
        func = vTable.write_vfunc(0x4, &ReadyWeaponHandlerHook::thunk);
    }
    void ReadyWeaponHandlerHook::thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
    {
		if (!a_this || !a_event || !a_data) return func(a_this, a_event, a_data);

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player->AsActorState()->IsWeaponDrawn()) {
			return func(a_this, a_event, a_data);
        }

		if (!a_event->IsUp()) return;
        if (a_event->HeldDuration() < held_threshold) {
			a_event->value = 1.0f;
			a_event->heldDownSecs = 0.f;
            return func(a_this, a_event, a_data);
        }

		auto weapon_L = player->GetEquippedObject(true);
		RE::TESForm* weapon = weapon_L ? weapon_L : player->GetEquippedObject(false);
		if (!weapon || !weapon->IsWeapon()) return func(a_this, a_event, a_data);

		SKSE::GetTaskInterface()->AddTask([player, weapon]() {
			auto bound = weapon->As<RE::TESBoundObject>();
            RE::ActorEquipManager::GetSingleton()->UnequipObject(player,bound);
            player->RemoveItem(bound,1,RE::ITEM_REMOVE_REASON::kDropping,nullptr,nullptr);
		});

		return func(a_this, a_event, a_data);
    }

    void Install()
    {
		ReadyWeaponHandlerHook::InstallHook();
    }
};
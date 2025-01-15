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

    
    void EquipObjectOverRide::InstallHook() {
        REL::Relocation<std::uintptr_t> target{RELOCATION_ID(37938, 38894), REL::VariantOffset(0xE5, 0x170, 0xE5)};

        auto& trampoline = SKSE::GetTrampoline();
        SKSE::AllocTrampoline(14);
        EquipObjectOverRide::func = trampoline.write_call<5>(target.address(), EquipObjectOverRide::thunk);
    }

    void EquipObjectOverRide::thunk(RE::ActorEquipManager* a_self, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                    std::uint64_t a_unk) {

        if (!a_object || !a_actor || !a_self) {
            func(a_self, a_actor, a_object, a_unk);
            return;
        }

        logger::debug("{} is equiping {}", a_actor->GetName(), a_object->GetName());
        if (a_object->IsWeapon()) {  // It's a weapon
            logger::debug("It's a Weapon!");
            RE::ActorState* actorState = a_actor->AsActorState();
            if (actorState) {                       // nullptr check
                if (actorState->IsWeaponDrawn()) {  // He has weapon ready (kDrawn || kWantToSheathe ||
                                                    // kSheathing) TBD: Limit it to just kDrawn??
                    
                    RE::TESForm* RequippedObject = a_actor->GetEquippedObject(false);
                    RE::TESForm* LequippedObject = a_actor->GetEquippedObject(true);

                    if (RequippedObject && RequippedObject->Is(RE::FormType::Weapon)) {
                        RE::TESObjectWEAP* weapon = skyrim_cast<RE::TESObjectWEAP*>(RequippedObject);
                        logger::info("Right Hand {}", weapon->GetName());

                        a_actor->DrawWeaponMagicHands(false);
                        return;
                    }
                    if (LequippedObject 
                    && (LequippedObject->Is(RE::FormType::Weapon) 
                    || LequippedObject->Is(RE::FormType::Armor))) { // Shield
                        RE::TESObjectWEAP* weapon = skyrim_cast<RE::TESObjectWEAP*>(RequippedObject);
                        logger::info("Left Hand {}", weapon->GetName());

                        a_actor->DrawWeaponMagicHands(false);
                        return;
                    }
                }
            }
        }
        func(a_self, a_actor, a_object, a_unk);
    }

    void Install()
    {
		ReadyWeaponHandlerHook::InstallHook();
        EquipObjectOverRide::InstallHook();
    }
};
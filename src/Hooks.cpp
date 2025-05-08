
#include "Animation.h"
#include "Events.h"
#include "Hooks.h"
#include "Settings.h"
#include "Utils.h"

namespace Hooks {

    void Install() {
        constexpr size_t size_per_hook = 14;
        auto& trampoline = SKSE::GetTrampoline();

        SKSE::AllocTrampoline(size_per_hook * 4);

        EquipObjectHook::InstallHook(trampoline);
        EquipSpellHook::InstallHook(trampoline);
        UnEquipObjectHook::InstallHook(trampoline);
        InputHook::InstallHook(trampoline);

        logger::info("Hooks Installed");
    }

    void EquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // Commonlib
        func = a_trampoline.write_call<5>(REL::RelocationID(37938, 38894).address() + REL::Relocate(0xE5, 0x170), thunk);
    }
    void EquipSpellHook::InstallHook(SKSE::Trampoline& a_trampoline) {

        // Commonlib
        func = a_trampoline.write_call<5>(REL::RelocationID(37939, 38895).address() + REL::Relocate(0x47, 0x47), thunk);
    }
    void UnEquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // Commonlib
        func = a_trampoline.write_call<5>(REL::RelocationID(37945, 38901).address() + REL::Relocate(0x138, 0x1b9), thunk);
    }

    void InputHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        func = a_trampoline.write_call<5>(REL::RelocationID(67315, 68617).address() + REL::Relocate(0x7B, 0x7B), thunk);
    }

    void EquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                std::uint64_t a_unk) {

        if (!a_actor || !a_object || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        // Check mod enabled for actor
        if ((!Settings::PC_Switch && a_actor->IsPlayerRef()) || (!Settings::NPC_Switch && !a_actor->IsPlayerRef())) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        if (!Utils::IsInHand(a_object)) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        logger::trace("Equip Hook: {} {}", a_actor->GetName(), a_object->GetName());

        if (const RE::ActorState* actorState = a_actor->AsActorState()) {
            if (actorState->GetWeaponState() != RE::WEAPON_STATE::kSheathed) {
                logger::trace("Equip Hook: Not kSheathed");

                bool left = false;
                if (a_actor->IsPlayerRef()) {
                    if (Utils::player_equip_left == true) {
                        left = true;
                        Utils::player_equip_left = false;
                    }
                } else { // NPC
                    //If right hand is weapon and left is empty, equip to left
                }

                if (Utils::IsLeftOnly(a_object)) {
                    left = true;
                }

                auto actor_right_hand = a_actor->GetEquippedObject(false);
                auto actor_left_hand = a_actor->GetEquippedObject(true);

                // if equip to empty hand
                if (left && !actor_left_hand) {
                    return func(a_manager, a_actor, a_object, a_unk);
                }
                if (!left && !actor_right_hand) {
                    return func(a_manager, a_actor, a_object, a_unk);
                }

                CreateEventSink(a_actor);
                Utils::UpdateEventInfo(a_actor, a_object, left, false);

                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }

        }
        return func(a_manager, a_actor, a_object, a_unk);
    }

    void EquipSpellHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::SpellItem* a_spell,
                               RE::BGSEquipSlot** a_slot) {
        if (!a_spell || !a_actor || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_spell, a_slot);
        }

        // Check mod enabled for actor
        if ((!Settings::PC_Switch && a_actor->IsPlayerRef()) || (!Settings::NPC_Switch && !a_actor->IsPlayerRef())) {
            return func(a_manager, a_actor, a_spell, a_slot);
        }
        logger::trace("Equip Spell Hook: {} {}", a_actor->GetName(), a_spell->GetName());

        return func(a_manager, a_actor, a_spell, a_slot);
    }

    void UnEquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                  std::uint64_t a_unk) {
        if (!a_actor || !a_object || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        // Check mod enabled for actor
        if ((!Settings::PC_Switch && a_actor->IsPlayerRef()) || (!Settings::NPC_Switch && !a_actor->IsPlayerRef())) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        if (a_object->IsWeapon()) {
            if ((a_object == Utils::unarmed_weapon) || (a_object->As<RE::TESObjectWEAP>()->IsBound())) {
                return func(a_manager, a_actor, a_object, a_unk);
            }
        }

        if (Utils::IsWhitelistUnequip(a_object)) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        logger::trace("UnEquip Hook: {} {}", a_actor->GetName(), a_object->GetName());

        return func(a_manager, a_actor, a_object, a_unk);
    }

    void InputHook::thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_event) {
        if (!Settings::Mod_Active || !a_dispatcher || !a_event) {
            return func(a_dispatcher, a_event);
        }
        auto first = *a_event;
        auto last = *a_event;
        size_t length = 0;

        for (auto current = *a_event; current; current = current->next) {
            ProcessInput(current);
            last = current;
            ++length;
        }

        if (length == 0) {
            constexpr RE::InputEvent* const dummy[] = {nullptr};
            func(a_dispatcher, dummy);
        } else {
            RE::InputEvent* const e[] = {first};
            func(a_dispatcher, e);
        }
    }

    void InputHook::ProcessInput(RE::InputEvent* event) {
        if (const auto button_event = event->AsButtonEvent()) {
            if (button_event->userEvent == RE::UserEvents::GetSingleton()->leftEquip ||
                button_event->userEvent == RE::UserEvents::GetSingleton()->leftAttack) {
                if (button_event->IsPressed()) {
                    Utils::player_equip_left = true;
                }
            }
            if (button_event->userEvent == RE::UserEvents::GetSingleton()->rightEquip ||
                button_event->userEvent == RE::UserEvents::GetSingleton()->rightAttack) {
                if (button_event->IsPressed()) {
                    Utils::player_equip_left = false;
                }
            }
        }
    }
};


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

        RemoveItemHook<RE::PlayerCharacter>::InstallHook();

        logger::info("Hooks Installed");
    }

    void EquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // All Calls
        func = a_trampoline.write_call<5>(REL::RelocationID(37938, 38894).address() + REL::Relocate(0xE5, 0x170), thunk);
    }

    void EquipSpellHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // Menus
        func = a_trampoline.write_call<5>(REL::RelocationID(37952, 38908).address() + REL::Relocate(0xd7, 0xd7), thunk);
        // Hotkey
        func = a_trampoline.write_call<5>(REL::RelocationID(37950, 38906).address() + REL::Relocate(0xc5, 0xca), thunk); 
        // Commonlib
        func = a_trampoline.write_call<5>(REL::RelocationID(37939, 38895).address() + REL::Relocate(0x47, 0x47), thunk);
    }

    void UnEquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // All Calls
        func = a_trampoline.write_call<5>(REL::RelocationID(37945, 38901).address() + REL::Relocate(0x138, 0x1b9), thunk);
    }

    void InputHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        func = a_trampoline.write_call<5>(REL::RelocationID(67315, 68617).address() + REL::Relocate(0x7B, 0x7B), thunk);
    }

    template <class T>
    void RemoveItemHook<T>::InstallHook() {
        REL::Relocation<std::uintptr_t> vTable(T::VTABLE[0]);
        func = vTable.write_vfunc(0x56, &RemoveItemHook::thunk);
        logger::debug("Installed RemoveItemHook<{}>", typeid(T).name());
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

        if (a_actor->IsDead()) { // :(
            return func(a_manager, a_actor, a_object, a_unk);
        }

        if (!Utils::IsInHand(a_object)) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        logger::debug("[Equip Hook]:[{} - {}] {}", a_actor->GetName(), a_actor->GetFormID(), a_object->GetName());

        if (const RE::ActorState* actorState = a_actor->AsActorState()) {
            if (actorState->IsWeaponDrawn() || Utils::IsAlreadyTracked(a_actor)) {
                logger::debug("[Equip Hook]:[{} - {}] Weapon State {}", a_actor->GetName(), a_actor->GetFormID(),
                              Helper::WeaponStateToString(actorState->GetWeaponState()));

                auto actor_right_hand = a_actor->GetEquippedObject(false);
                auto actor_left_hand = a_actor->GetEquippedObject(true);

                bool left = false;
                if (a_actor->IsPlayerRef()) {
                    if (Utils::player_equip_left == true) {
                        left = true;
                        Utils::player_equip_left = false;
                    }
                } else { // NPC
                    if (actor_right_hand && !actor_left_hand) {
                        left = true;
                    }
                    if (Utils::IsAlreadyTracked(a_actor)) {
                        auto EQEvent = Utils::GetEvent(a_actor);
                        if (EQEvent.right && !EQEvent.left && a_object != EQEvent.right) {
                            left = true;
                        }
                    }
                }

                if (Utils::IsLeftOnly(a_object)) {
                    left = true;
                }

                // if equip to empty hand
                std::pair<bool, bool> hands_empty = Utils::GetIfHandsEmpty(a_actor);
                bool left_empty = hands_empty.second;
                bool right_empty = hands_empty.first;

                if (Utils::IsTwoHanded(a_object)) {
                    if (right_empty && left_empty) {
                        return func(a_manager, a_actor, a_object, a_unk);
                    } else if (a_object->IsWeapon() && a_object->As<RE::TESObjectWEAP>()->IsBound()) {
                        RE::DebugNotification("You can't summon this weapon - you need both hands free!", "UIMenuCancel");
                        return;
                    }
                } else {
                    if (left && left_empty) {
                        return func(a_manager, a_actor, a_object, a_unk);
                    } else if (!left && right_empty) {
                        return func(a_manager, a_actor, a_object, a_unk);
                    }
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

        if (a_spell->data.spellType != RE::MagicSystem::SpellType::kSpell) {
            return func(a_manager, a_actor, a_spell, a_slot);
        }

        // Check mod enabled for actor
        if ((!Settings::PC_Switch && a_actor->IsPlayerRef()) || (!Settings::NPC_Switch && !a_actor->IsPlayerRef())) {
            return func(a_manager, a_actor, a_spell, a_slot);
        }

        logger::debug("[Equip Spell Hook]:[{} - {}] {} {}", a_actor->GetName(), a_actor->GetFormID(),
                      a_spell->GetName(), *a_slot == Utils::left_hand_slot ? "left" : "right");


        if (const RE::ActorState* actorState = a_actor->AsActorState()) {
            if (actorState->IsWeaponDrawn() || Utils::IsAlreadyTracked(a_actor)) {
                logger::debug("[Equip Spell Hook]:[{} - {}] Weapon State {}", a_actor->GetName(), a_actor->GetFormID(),
                              Helper::WeaponStateToString(actorState->GetWeaponState()));

                // if equip to empty hand
                std::pair<bool, bool> hands_empty = Utils::GetIfHandsEmpty(a_actor);
                bool left_empty = hands_empty.second;
                bool right_empty = hands_empty.first;

                if (*a_slot == Utils::left_hand_slot && left_empty) {
                    return func(a_manager, a_actor, a_spell, a_slot);
                }
                if ((*a_slot == Utils::right_hand_slot || !*a_slot) && right_empty) {
                    return func(a_manager, a_actor, a_spell, a_slot);
                }

                CreateEventSink(a_actor);
                Utils::UpdateEventInfo(a_actor, a_spell, *a_slot == Utils::left_hand_slot, false);

                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }
        }

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

        if (a_actor->IsDead()) {  // :(
            return func(a_manager, a_actor, a_object, a_unk);
        }

        if (Utils::remove_obj == a_object && Utils::remove_act == a_actor) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        if (!Utils::IsInHand(a_object)) {
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

        logger::debug("[UnEquip Hook]:[{} - {}] ", a_actor->GetName(), a_actor->GetFormID(), a_object->GetName());

        if (const RE::ActorState* actorState = a_actor->AsActorState()) {
            if (actorState->IsWeaponDrawn() || Utils::IsAlreadyTracked(a_actor)) {
                logger::debug("[Equip Hook]:[{} - {}] Weapon State {}", a_actor->GetName(), a_actor->GetFormID(),
                              Helper::WeaponStateToString(actorState->GetWeaponState()));

                bool left = false;
                if (a_actor->IsPlayerRef()) {
                    if (Utils::player_equip_left == true) {
                        left = true;
                        Utils::player_equip_left = false;
                    }
                } else {  // NPC
                }

                if (Utils::IsLeftOnly(a_object)) {
                    left = true;
                }

                CreateEventSink(a_actor);
                Utils::UpdateEventInfo(a_actor, a_object, left, true);

                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }
        }

        return func(a_manager, a_actor, a_object, a_unk);
    }

    template <class T>
    RE::ObjectRefHandle* RemoveItemHook<T>::thunk(T* a_this, RE::ObjectRefHandle& a_hidden_return_argument,
                                             RE::TESBoundObject* a_item,
                               std::int32_t a_count, RE::ITEM_REMOVE_REASON a_reason, RE::ExtraDataList* a_extra_list,
                               RE::TESObjectREFR* a_move_to_ref, const RE::NiPoint3* a_drop_loc,
                               const RE::NiPoint3* a_rotate) {
        logger::debug("[RemoveItemHook<{}>] [{} - {}] removing {}", typeid(T).name(), a_this->GetName(),
                      a_this->GetFormID(), a_item->GetName());

        Utils::remove_act = a_this;
        Utils::remove_obj = a_item;

        return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extra_list, a_move_to_ref,
                    a_drop_loc, a_rotate);
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
            } else if (button_event->userEvent == RE::UserEvents::GetSingleton()->rightEquip ||
                       button_event->userEvent == RE::UserEvents::GetSingleton()->rightAttack) {
                if (button_event->IsPressed()) {
                    Utils::player_equip_left = false;
                }
            } else {
                if (button_event->IsPressed()) {
                    Utils::player_equip_left = false;
                }
            }
        }
    }
};


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

        GenericEquipObjectHook::InstallHook(trampoline);
        EquipObjectHook::InstallHook(trampoline);
        EquipSpellHook::InstallHook(trampoline);
        UnEquipObjectHook::InstallHook(trampoline);

        logger::info("Hooks Installed");
    }

    void InstallReadOnly() {
        constexpr size_t size_per_hook = 14;
        auto& trampoline = SKSE::GetTrampoline();

        SKSE::AllocTrampoline(size_per_hook * 1);

        InputHook::InstallHook(trampoline);

        RemoveItemHook<RE::PlayerCharacter>::InstallHook();
        RemoveItemHook<RE::Character>::InstallHook();

        logger::info("Read Only Hooks Installed");

    }

    void GenericEquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // All Calls
        func = a_trampoline.write_call<5>(REL::RelocationID(37938, 38894).address() + REL::Relocate(0xE5, 0x170), thunk);
    }

    void EquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        // PC
        func = a_trampoline.write_call<5>(REL::RelocationID(37951, 38907).address() + REL::Relocate(0x2e0, 0x2e0), thunk);
        // NPC
        func = a_trampoline.write_call<5>(REL::RelocationID(46955, 48124).address() + REL::Relocate(0x1a5, 0x1d6), thunk);
        // Brawl (Papyrus?)
        func = a_trampoline.write_call<5>(REL::RelocationID(53861, 54661).address() + REL::Relocate(0x14e, 0x14e), thunk);
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

    void GenericEquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor,
                                       RE::TESBoundObject* a_object,
                                std::uint64_t a_unk) {

        if (!a_actor || !a_object || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        RE::TESObjectREFR::Count count = 0;
        if (a_object->IsWeapon()) {
            RE::TESObjectREFR::InventoryItemMap inv = a_actor->GetInventory();
            auto it_inv = inv.find(a_object);
            if (it_inv != inv.end()) {
                count = it_inv->second.first;
            } else {
                logger::error("[Generic Equip Hook]:[{} - {:08X}] {} not found in inventory.", a_actor->GetName(),
                              a_actor->GetFormID(), a_object->GetName());
            }
        }

        const bool isWorn =
            a_object == a_actor->GetEquippedObject(false) || a_object == a_actor->GetEquippedObject(true);
        const bool isPlayer = a_actor->IsPlayerRef();
        const bool pcSwitchDisabled = !Settings::PC_Switch && isPlayer;
        const bool npcSwitchDisabled = !Settings::NPC_Switch && !isPlayer;
        const bool isDead = a_actor->IsDead();
        const bool notInHand = !Utils::IsInHand(a_object);
        const bool alreadyEquiped = count == 1 && isWorn;

        logger::debug("[Generic Equip Hook]:[{} - {:08X}] Pass flags {} {} {} {} {}", a_actor->GetName(), a_actor->GetFormID(),
                      pcSwitchDisabled, npcSwitchDisabled, isDead, notInHand, alreadyEquiped);
        if (pcSwitchDisabled || npcSwitchDisabled || isDead || notInHand || alreadyEquiped) {
            Utils::justEquiped_act = a_actor;
            Utils::justEquiped_obj = a_object;
            Utils::justEquiped_time = std::chrono::steady_clock::now();
            return func(a_manager, a_actor, a_object, a_unk);
        }

        logger::debug("[Generic Equip Hook]:[{} - {:08X}] {}", a_actor->GetName(), a_actor->GetFormID(), a_object->GetName());

        if (const RE::ActorState* actorState = a_actor->AsActorState()) {
            if (actorState->IsWeaponDrawn() || Utils::IsAlreadyTracked(a_actor)) {
                logger::debug("[Generic Equip Hook]:[{} - {:08X}] Weapon State {} | {}tracked", a_actor->GetName(),
                              a_actor->GetFormID(), Helper::WeaponStateToString(actorState->GetWeaponState()),
                              Utils::IsAlreadyTracked(a_actor) ? "" : "NOT");

                
                // if equip to empty hand
                std::pair<bool, bool> hands_empty = Utils::GetIfHandsEmpty(a_actor);
                bool left_empty = hands_empty.second;
                bool right_empty = hands_empty.first;

                bool left = false;
                if (a_actor->IsPlayerRef()) {
                    if (Utils::player_equip_left == true) {
                        left = true;
                        Utils::player_equip_left = false;
                    }
                }

                if (Utils::IsLeftOnly(a_object)) {
                    left = true;
                }

                if (Utils::IsTwoHanded(a_object)) {
                    if (right_empty && left_empty) {
                        if (!Utils::IsAlreadyTracked(a_actor)) {
                            Utils::justEquiped_act = a_actor;
                            Utils::justEquiped_obj = a_object;
                            Utils::justEquiped_time = std::chrono::steady_clock::now();
                            return func(a_manager, a_actor, a_object, a_unk);
                        }
                    } else if (a_object->IsWeapon() && a_object->As<RE::TESObjectWEAP>()->IsBound()) {
                        if (a_actor->IsPlayerRef()) {
                            RE::DebugNotification("You can't summon this weapon - you need both hands free!",
                                                  "UIMenuCancel");
                        }
                        return;
                    }
                    Utils::SetAnimationInfo(a_actor, false, false);
                    Utils::SetAnimationInfo(a_actor, true, false);
                } else {
                    if (a_actor->IsPlayerRef()) {
                        if (left && left_empty) {
                            if (!Utils::IsAlreadyTracked(a_actor)) {
                                Utils::SetAnimationInfo(a_actor, left, true);
                                auto eventSink = GetOrCreateEventSink(a_actor);
                                Utils::justEquiped_act = a_actor;
                                Utils::justEquiped_obj = a_object;
                                Utils::justEquiped_time = std::chrono::steady_clock::now();
                                return func(a_manager, a_actor, a_object, a_unk);
                            }
                        }
                    }
                    if (!left && right_empty) {
                        if (!Utils::IsAlreadyTracked(a_actor)) {
                            Utils::SetAnimationInfo(a_actor, left, true);
                            auto eventSink = GetOrCreateEventSink(a_actor);
                            Utils::justEquiped_act = a_actor;
                            Utils::justEquiped_obj = a_object;
                            Utils::justEquiped_time = std::chrono::steady_clock::now();
                            return func(a_manager, a_actor, a_object, a_unk);
                        }
                    }
                }

                auto currentRight = a_actor->GetEquippedObject(false);
                if (Utils::IsTwoHanded(currentRight)) {
                    Utils::SetAnimationInfo(a_actor, false, false);
                    Utils::SetAnimationInfo(a_actor, true, false);
                } else {
                    Utils::SetAnimationInfo(a_actor, left, false);
                }
                auto eventSink = GetOrCreateEventSink(a_actor);
                Utils::UpdateEventInfo(a_actor, a_object, left, false, eventSink);

                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }
        }
        Utils::justEquiped_act = a_actor;
        Utils::justEquiped_obj = a_object;
        Utils::justEquiped_time = std::chrono::steady_clock::now();
        return func(a_manager, a_actor, a_object, a_unk);
    }

    void EquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                RE::ExtraDataList* a_extraData, std::uint32_t a_count,
                                const RE::BGSEquipSlot* a_slot, bool a_queueEquip,
                                bool a_forceEquip, bool a_playSounds, bool a_applyNow) {
        if (!a_actor || !a_object || !a_manager || !Settings::Mod_Active) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow);
        }

        RE::TESObjectREFR::Count count = 0;
        if (a_object->IsWeapon()) {
            RE::TESObjectREFR::InventoryItemMap inv = a_actor->GetInventory();
            auto it_inv = inv.find(a_object);
            if (it_inv != inv.end()) {
                count = it_inv->second.first;
            } else {
                logger::error("[Equip Hook]:[{} - {:08X}] {} not found in inventory.", a_actor->GetName(),
                              a_actor->GetFormID(), a_object->GetName());
            }
        }

        const bool isWorn =
            a_object == a_actor->GetEquippedObject(false) || a_object == a_actor->GetEquippedObject(true);
        const bool isPlayer = a_actor->IsPlayerRef();
        const bool pcSwitchDisabled = !Settings::PC_Switch && isPlayer;
        const bool npcSwitchDisabled = !Settings::NPC_Switch && !isPlayer;
        const bool isDead = a_actor->IsDead();
        const bool notInHand = !Utils::IsInHand(a_object);
        const bool alreadyEquiped = count == 1 && isWorn;

        logger::debug("[Equip Hook]:[{} - {:08X}] Pass flags {} {} {} {} {}", a_actor->GetName(), a_actor->GetFormID(),
                      pcSwitchDisabled, npcSwitchDisabled, isDead, notInHand, alreadyEquiped);
        if (pcSwitchDisabled || npcSwitchDisabled || isDead || notInHand || alreadyEquiped) {
            Utils::justEquiped_act = a_actor;
            Utils::justEquiped_obj = a_object;
            Utils::justEquiped_time = std::chrono::steady_clock::now();
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow);
        }

        logger::debug("[Equip Hook]:[{} - {:08X}] {}", a_actor->GetName(), a_actor->GetFormID(), a_object->GetName());

        if (const RE::ActorState* actorState = a_actor->AsActorState()) {
            if (actorState->IsWeaponDrawn() || Utils::IsAlreadyTracked(a_actor)) {
                logger::debug("[Equip Hook]:[{} - {:08X}] Weapon State {} | {}tracked", a_actor->GetName(),
                              a_actor->GetFormID(), Helper::WeaponStateToString(actorState->GetWeaponState()),
                              Utils::IsAlreadyTracked(a_actor) ? "" : "NOT");

                // if equip to empty hand
                std::pair<bool, bool> hands_empty = Utils::GetIfHandsEmpty(a_actor);
                bool left_empty = hands_empty.second;
                bool right_empty = hands_empty.first;

                bool left = false;
                if (a_actor->IsPlayerRef()) {
                    if (Utils::player_equip_left == true) {
                        left = true;
                        Utils::player_equip_left = false;
                    }
                }
                if (a_slot && a_slot->GetFormID() == Utils::EquipSlots::Left) {
                    left = true;
                }

                if (Utils::IsLeftOnly(a_object)) {
                    left = true;
                }

                if (Utils::IsTwoHanded(a_object)) {
                    if (right_empty && left_empty) {
                        if (!Utils::IsAlreadyTracked(a_actor)) {
                            Utils::justEquiped_act = a_actor;
                            Utils::justEquiped_obj = a_object;
                            Utils::justEquiped_time = std::chrono::steady_clock::now();
                            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip,
                                        a_forceEquip, a_playSounds, a_applyNow);
                        }
                    } else if (a_object->IsWeapon() && a_object->As<RE::TESObjectWEAP>()->IsBound()) {
                        if (a_actor->IsPlayerRef()) {
                            RE::DebugNotification("You can't summon this weapon - you need both hands free!",
                                                  "UIMenuCancel");
                        }
                        return;
                    }
                    Utils::SetAnimationInfo(a_actor, false, false);
                    Utils::SetAnimationInfo(a_actor, true, false);
                } else {
                    if (a_actor->IsPlayerRef()) {
                        if (left && left_empty) {
                            if (!Utils::IsAlreadyTracked(a_actor)) {
                                Utils::SetAnimationInfo(a_actor, left, true);
                                auto eventSink = GetOrCreateEventSink(a_actor);
                                Utils::justEquiped_act = a_actor;
                                Utils::justEquiped_obj = a_object;
                                Utils::justEquiped_time = std::chrono::steady_clock::now();
                                return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip,
                                            a_forceEquip, a_playSounds, a_applyNow);
                            }
                        }
                    }
                    if (!left && right_empty) {
                        if (!Utils::IsAlreadyTracked(a_actor)) {
                            Utils::SetAnimationInfo(a_actor, left, true);
                            auto eventSink = GetOrCreateEventSink(a_actor);
                            Utils::justEquiped_act = a_actor;
                            Utils::justEquiped_obj = a_object;
                            Utils::justEquiped_time = std::chrono::steady_clock::now();
                            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip,
                                        a_forceEquip, a_playSounds, a_applyNow);
                        }
                    }
                }

                auto currentRight = a_actor->GetEquippedObject(false);
                if (Utils::IsTwoHanded(currentRight)) {
                    Utils::SetAnimationInfo(a_actor, false, false);
                    Utils::SetAnimationInfo(a_actor, true, false);
                } else {
                    Utils::SetAnimationInfo(a_actor, left, false);
                }
                auto eventSink = GetOrCreateEventSink(a_actor);
                Utils::UpdateEventInfo(a_actor, a_object, left, false, eventSink);

                a_actor->DrawWeaponMagicHands(false);
                a_actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                return;
            }
        }
        Utils::justEquiped_act = a_actor;
        Utils::justEquiped_obj = a_object;
        Utils::justEquiped_time = std::chrono::steady_clock::now();
        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow);
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

        logger::debug("[Equip Spell Hook]:[{} - {:08X}] {} {}", a_actor->GetName(), a_actor->GetFormID(),
                      a_spell->GetName(), *a_slot == Utils::left_hand_slot ? "left" : "right");

        if (const RE::ActorState* actorState = a_actor->AsActorState()) {
            if (actorState->IsWeaponDrawn() || Utils::IsAlreadyTracked(a_actor)) {
                logger::debug("[Equip Spell Hook]:[{} - {:08X}] Weapon State {} | {}tracked", a_actor->GetName(),
                              a_actor->GetFormID(), Helper::WeaponStateToString(actorState->GetWeaponState()),
                              Utils::IsAlreadyTracked(a_actor) ? "" : "NOT");

                // if equip to empty hand
                std::pair<bool, bool> hands_empty = Utils::GetIfHandsEmpty(a_actor);
                bool left_empty = hands_empty.second;
                bool right_empty = hands_empty.first;

                if (Utils::IsTwoHanded(a_spell)) {
                    if (left_empty && right_empty) {
                        if (!Utils::IsAlreadyTracked(a_actor)) {
                            return func(a_manager, a_actor, a_spell, a_slot);
                        }
                    }
                    Utils::SetAnimationInfo(a_actor, true, false);
                    Utils::SetAnimationInfo(a_actor, false, false);
                } else {
                    if (*a_slot == Utils::left_hand_slot && left_empty) {
                        if (!Utils::IsAlreadyTracked(a_actor)) {
                            return func(a_manager, a_actor, a_spell, a_slot);
                        }
                    }
                    if ((*a_slot == Utils::right_hand_slot || !*a_slot) && right_empty) {
                        if (!Utils::IsAlreadyTracked(a_actor)) {
                            return func(a_manager, a_actor, a_spell, a_slot);
                        }
                    }
                    Utils::SetAnimationInfo(a_actor, *a_slot == Utils::left_hand_slot, false);
                }

                auto eventSink = GetOrCreateEventSink(a_actor);
                Utils::UpdateEventInfo(a_actor, a_spell, *a_slot == Utils::left_hand_slot, false, eventSink);

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
        auto now = std::chrono::steady_clock::now();

        const bool isPlayer = a_actor->IsPlayerRef();
        const bool isDead = a_actor->IsDead();
        const bool removeTreshold =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - Utils::remove_time).count() < 50;
        const bool justEquipTreshold =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - Utils::justEquiped_time).count() < 50;
        const bool isRemoveUnequip = (Utils::remove_obj == a_object && Utils::remove_act == a_actor && removeTreshold);
        const bool isJustEquip = (Utils::justEquiped_obj == a_object && Utils::justEquiped_act == a_actor && justEquipTreshold);
        const bool notInHand = !Utils::IsInHand(a_object);
        const bool isWeapon = a_object->IsWeapon();
        const auto weap = a_object->As<RE::TESObjectWEAP>();
        const bool isUnarmedWeapon = (a_object == Utils::unarmed_weapon);
        const bool isBoundWeapon = (weap && weap->IsBound());
        const bool isWhitelisted = Utils::IsWhitelistUnequip(a_object);
        const bool switchingNotAllowed = (!Settings::PC_Switch && isPlayer) || (!Settings::NPC_Switch && !isPlayer);

        logger::debug("[UnEquip Hook]:[{} - {:08X}] Pass flags {} {} {} {} {} {}", a_actor->GetName(),
                      a_actor->GetFormID(), switchingNotAllowed, isDead, isRemoveUnequip, isJustEquip,
                      (isWeapon && (isUnarmedWeapon || isBoundWeapon)), isWhitelisted);

        if (switchingNotAllowed || isDead || isRemoveUnequip || isJustEquip || notInHand ||
            (isWeapon && (isUnarmedWeapon || isBoundWeapon)) || isWhitelisted) {
            return func(a_manager, a_actor, a_object, a_unk);
        }

        // Deadlock breaker to detect rapid unequip spam
        // This happens in rare cases such as NPC transformation into the werewolf
        static RE::Actor* last_actor = nullptr;
        static RE::TESBoundObject* last_object = nullptr;
        static std::chrono::steady_clock::time_point last_time;
        static int spam_counter = 0;

        if (a_actor == last_actor && a_object == last_object) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() < 50) {
                spam_counter++;
            } else {
                spam_counter = 0;
            }
        } else {
            spam_counter = 0;
        }

        last_actor = a_actor;
        last_object = a_object;
        last_time = now;

        if (spam_counter > 5) {
            logger::warn("[UnEquip Hook]:[{} - {:08X}] Detected unequip loop allowing unequip to break deadlock",
                         a_actor->GetName(), a_actor->GetFormID());
            if (Utils::IsAlreadyTracked(a_actor)) {
                Utils::RemoveEvent(a_actor);
            }
            return func(a_manager, a_actor, a_object, a_unk);
        }

        logger::debug("[UnEquip Hook]:[{} - {:08X}] {}", a_actor->GetName(), a_actor->GetFormID(), a_object->GetName());

        if (const RE::ActorState* actorState = a_actor->AsActorState()) {
            if (actorState->IsWeaponDrawn() || Utils::IsAlreadyTracked(a_actor)) {
                logger::debug("[UnEquip Hook]:[{} - {:08X}] Weapon State {} | {}tracked", a_actor->GetName(),
                              a_actor->GetFormID(), Helper::WeaponStateToString(actorState->GetWeaponState()),
                              Utils::IsAlreadyTracked(a_actor) ? "" : "NOT");


                bool left = false;
                bool hand_switch = false;
                if (a_actor->IsPlayerRef()) {
                    if (Utils::player_equip_left == true) {
                        left = true;
                        Utils::player_equip_left = false;
                    }
                    if (!Utils::IsTwoHanded(a_object) && a_object == a_actor->GetEquippedObject(!left)) {
                        RE::TESObjectREFR::Count count = 0;
                        if (a_object->IsWeapon()) {
                            RE::TESObjectREFR::InventoryItemMap inv = a_actor->GetInventory();
                            auto it_inv = inv.find(a_object);
                            if (it_inv != inv.end()) {
                                count = it_inv->second.first;
                            } else {
                                logger::error("[Equip Hook]:[{} - {:08X}] {} not found in inventory.",
                                              a_actor->GetName(), a_actor->GetFormID(), a_object->GetName());
                            }
                        }
                        if (count == 1) {  // swithing from one hand to another
                            Utils::SetAnimationInfoHandSwitch(a_actor);
                            hand_switch = true;
                        }
                    }
                } else {
                    if (a_object == a_actor->GetEquippedObject(true)) {
                        left = true;
                    }
                }

                if (Utils::IsLeftOnly(a_object)) {
                    left = true;
                }

                if (Utils::IsTwoHanded(a_object)) {
                    Utils::SetAnimationInfo(a_actor, false, false);
                    Utils::SetAnimationInfo(a_actor, true, false);
                } else {
                    Utils::SetAnimationInfo(a_actor, left, false);
                }
                auto eventSink = GetOrCreateEventSink(a_actor);
                if (hand_switch) {
                    Utils::UpdateEventInfo(a_actor, a_object, !left, true, eventSink);
                    Utils::UpdateEventInfo(a_actor, a_object, left, false, eventSink);
                } else {
                    Utils::UpdateEventInfo(a_actor, a_object, left, true, eventSink);
                }

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

        if (!a_this || !a_item) {
            return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extra_list, a_move_to_ref,
                        a_drop_loc, a_rotate);
        }
        logger::debug("[RemoveItemHook<{}>] [{} - {:08X}] removing {}", typeid(T).name(), a_this->GetName(),
                      a_this->GetFormID(), a_item->GetName());

        Utils::remove_act = a_this;
        Utils::remove_obj = a_item;
        Utils::remove_time = std::chrono::steady_clock::now();

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

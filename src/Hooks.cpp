#include "Hooks.h"
#include "Utils.h"


// ToDo and Ideas
//
// Bug (crutial)
// For some reason NPC do it like this:
// Equip animation - Old weapon  <--- Wo don't want this
// Unequip animation - Old weapon
// Equip Animation - New weapon
//
// Implement this for stawes as optional file? (For mods that add stawes equip/unequip animations)
// 
// General/New hooks?
// ToDo: Inventory showing what item will be quiped (like it's equiped now)
// ToFix: Unable to dual Hand Equip
// ToFix: Able to equip shield when bow is equiped (bow is not auto uneqiped)
//
// When PC start combat (in town), and change weapon NPC stop combat
// Because NPC stop combat when PC is Sheatling a weapon

// Not Working as expected use cases:
// 
// Player have magic two hands Ready position (kDrawn)
// Player equip a weaopn
// Player unequip a weapon
// Expected: Player equip spells
// Actual: Player equip bare hands
//
// ReadyWeaponHandlerHook
// ToDo: Add some force to object (like in Grab an throw)
// ToDo: Call attack unarmed Letf or Right hand (Or some vanilla animation tah can fit)
// ToFix: CTD on dropping Weap xp (UnequipHook Causing Inf loop?)


namespace Hooks {

    void Install() {
        constexpr size_t size_per_hook = 14;
        auto& trampoline = SKSE::GetTrampoline();
        SKSE::AllocTrampoline(size_per_hook * 2);

        ReadyWeaponHandlerHook::InstallHook();
        EquipObjectHook::InstallHook(trampoline);
        UnEquipObjectHook::InstallHook(trampoline);
        ActorUpdateHook<RE::Character>::InstallHook();
        ActorUpdateHook<RE::PlayerCharacter>::InstallHook();
    }

    void ReadyWeaponHandlerHook::InstallHook() {
        REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_ReadyWeaponHandler[0]);
        func = vTable.write_vfunc(0x4, &ReadyWeaponHandlerHook::thunk);
    }
    void EquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        const REL::Relocation<std::uintptr_t> target{RELOCATION_ID(37938, 38894),
                                                     REL::VariantOffset(0xE5, 0x170, 0xE5)};
        EquipObjectHook::func = a_trampoline.write_call<5>(target.address(), EquipObjectHook::thunk);
    }
    void UnEquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {
        const REL::Relocation<std::uintptr_t> target{RELOCATION_ID(37945, 38901),
                                                     REL::VariantOffset(0x138, 0x1B9, 0x138)};
        UnEquipObjectHook::func = a_trampoline.write_call<5>(target.address(), UnEquipObjectHook::thunk);
    }
    template <class T>
    void ActorUpdateHook<T>::InstallHook() {
        REL::Relocation<std::uintptr_t> vTable(T::VTABLE[0]);
        func = vTable.write_vfunc(0xAD, &ActorUpdateHook::thunk);
    }

    void ReadyWeaponHandlerHook::thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
    {
        if (!a_this || !a_event || !a_data) {
            return func(a_this, a_event, a_data);
        }

        RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
        if (!player->AsActorState()->IsWeaponDrawn()) {
			return func(a_this, a_event, a_data);
        }

		if (!a_event->IsUp()) return;
        if (a_event->HeldDuration() < held_threshold) {
			a_event->value = 1.0f;
			a_event->heldDownSecs = 0.f;
            return func(a_this, a_event, a_data);
        }

		RE::TESForm* weapon_L = player->GetEquippedObject(true);
		RE::TESForm* weapon_or_shield = weapon_L ? weapon_L : player->GetEquippedObject(false);
        if (!weapon_or_shield) {
            return func(a_this, a_event, a_data);
        }
        if (weapon_or_shield->IsWeapon() || weapon_or_shield->IsArmor()) {
            SKSE::GetTaskInterface()->AddTask([player, weapon_or_shield]() {
                RE::TESBoundObject* bound = weapon_or_shield->As<RE::TESBoundObject>();
                RE::ActorEquipManager::GetSingleton()->UnequipObject(player, bound);
                player->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
            });
        }

		return func(a_this, a_event, a_data);
    }

    

    void EquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                RE::ExtraDataList* a_extraData, std::uint32_t a_count, const RE::BGSEquipSlot* a_slot,
                                bool a_queueEquip, bool a_forceEquip, bool a_playSounds, bool a_applyNow) {

        if (!a_object || !a_actor || !a_manager) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow);
        }
        logger::trace("{} is Equiping {}", a_actor->GetName(), a_object->GetName());
        
        if (a_object->IsWeapon()) {
            if (Utils::IsInPospondEquipQueue(Utils::ActorInfo(a_actor))) {
                logger::trace("{} is in Pospond Equip Queue not allowing to Equip", a_actor->GetName());
                if (a_actor == RE::PlayerCharacter::GetSingleton()) {
                    Utils::UpdatePospondEquipQueue(a_actor, a_object);
                }
                return;
            }
            if (RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    RE::TESForm* RequippedObject = a_actor->GetEquippedObject(false);
                    RE::TESForm* LequippedObject = a_actor->GetEquippedObject(true);

                    if ((RequippedObject && RequippedObject->Is(RE::FormType::Weapon)) ||
                        (LequippedObject && LequippedObject->Is(RE::FormType::Weapon))) {
                        logger::trace("{} is in kDrawn Weapon State, adding to Pospond Equip Queue", a_actor->GetName());
                        Utils::UpdatePospondEquipQueue(a_actor, a_object);
                        a_actor->DrawWeaponMagicHands(false);
                        return;
                    }
                }
            }
        }
        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow);
    }

    void UnEquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                  RE::ExtraDataList* a_extraData, std::uint32_t a_count, const RE::BGSEquipSlot* a_slot,
                                  bool a_queueEquip, bool a_forceEquip, bool a_playSounds, bool a_applyNow,
                                  const RE::BGSEquipSlot* a_slotToReplace) {
        if (!a_object || !a_actor || !a_manager) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow, a_slotToReplace);
        }

        logger::trace("{} is Unequiping {}", a_actor->GetName(), a_object->GetName());

        if (a_object->IsWeapon()) {
            if (Utils::IsInPospondEquipQueue(Utils::ActorInfo(a_actor))) {
                logger::trace("{} is in Pospond Equip Queue not allowing to Unequip", a_actor->GetName());
                return;
            }
            if (RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    RE::TESForm* RequippedObject = a_actor->GetEquippedObject(false);
                    RE::TESForm* LequippedObject = a_actor->GetEquippedObject(true);

                    if ((RequippedObject && RequippedObject->Is(RE::FormType::Weapon)) ||
                        (LequippedObject && LequippedObject->Is(RE::FormType::Weapon))) {
                        logger::trace("{} is in kDrawn Weapon State, adding to Pospond Equip Queue",
                                      a_actor->GetName());
                        Utils::UpdatePospondEquipQueue(a_actor, a_object, true);
                        a_actor->DrawWeaponMagicHands(false);
                        return;
                    }
                }
            }
        }
        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow, a_slotToReplace);
    }

	template <class T>
    void ActorUpdateHook<T>::thunk(T* a_actor, float a_delta) {
        if (!a_actor) {
            return func(a_actor, a_delta);
        }

        const Utils::ActorInfo dummy_actor_info = Utils::ActorInfo(a_actor);
        if (!Utils::IsInPospondEquipQueue(dummy_actor_info)) {
            return func(a_actor, a_delta);
        }

        auto weapSts = a_actor->AsActorState()->GetWeaponState();

        
        RE::TESBoundObject* objectToEquip = Utils::GetObjectFromPospondEquipQueue(dummy_actor_info);

#ifndef NDEBUG
        RE::TESForm* currWeap = a_actor->GetEquippedObject(false);
        logger::trace("ActorInfo: {} CurrWeapon {} WeaponState {} ObjToEquip {}", a_actor->GetName(),
                      currWeap->GetName(), Helper::WeaponStateToString(weapSts), objectToEquip->GetName());
#endif

        // Dirty hack but somehow its working :D
        a_actor->DrawWeaponMagicHands(false);

        if (weapSts != RE::WEAPON_STATE::kSheathed) {
            return func(a_actor, a_delta);
        }

        if (objectToEquip) {
            if (Utils::GetUnequipFromPospondEquipQueue(dummy_actor_info)) {
                logger::trace("Removing {} from PospondEquipQueue, no weapon to Equip", a_actor->GetName());
                Utils::RemoveFromPospondEquipQueue(dummy_actor_info);
                RE::ActorEquipManager::GetSingleton()->UnequipObject(a_actor, objectToEquip);
                a_actor->DrawWeaponMagicHands(true);
            } else {
                logger::trace("Removing {} from PospondEquipQueue and Equiping new weapon", a_actor->GetName());
                Utils::RemoveFromPospondEquipQueue(dummy_actor_info);
                RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, objectToEquip);
                a_actor->DrawWeaponMagicHands(true);
            }
        }
        return func(a_actor, a_delta);
    }
};
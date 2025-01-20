#include "Hooks.h"
#include "Utils.h"


// ToDo and Ideas
//
// Implement for staves as optional? (For mods that add stawes equip/unequip animations)
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
// Expected: Player equip spells (like in vanilla)
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

        using func_t = decltype(&RE::ActorEquipManager::EquipObject);
        REL::Relocation<func_t> target{RE::Offset::ActorEquipManager::EquipObject,
                                       REL::VariantOffset(0xE5, 0x170, 0xE5)};
        //return func(this, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip, a_playSounds,  a_applyNow);

        //const REL::Relocation<std::uintptr_t> target{RE::Offset::ActorEquipManager::EquipObject};
        EquipObjectHook::func = a_trampoline.write_call<5>(target.address(), EquipObjectHook::thunk);
    }
    void UnEquipObjectHook::InstallHook(SKSE::Trampoline& a_trampoline) {

        using func_t = decltype(&RE::ActorEquipManager::UnequipObject);
        REL::Relocation<func_t> target{RE::Offset::ActorEquipManager::UnequipObject,
                                       REL::VariantOffset(0x138, 0x1B9, 0x138)};
        //return func(this, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip, a_playSounds, a_applyNow, a_slotToReplace);

        //const REL::Relocation<std::uintptr_t> target{RE::Offset::ActorEquipManager::UnequipObject};
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
                                bool a_queueEquip, bool a_forceEquip, bool a_playSounds, bool a_applyNow)
    {

        if (!a_object || !a_actor || !a_manager) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow);
        }
        logger::trace("{} is Equiping {}", a_actor->GetName(), a_object->GetName());
        logger::trace("a_slot {}", a_slot->GetFormID());
        
        if (a_object->IsWeapon()) { //Or Shield, Or Spell, Or Staff, Or Scroll
            if (Utils::IsInPospondQueue(a_actor->formID)) {
                logger::trace("{} is in Pospond Equip Queue not allowing to Equip", a_actor->GetName());
                if (a_actor == RE::PlayerCharacter::GetSingleton()) {
                    /*Utils::UpdatePospondQueue(a_actor->formID, [a_manager, a_actor, a_object, a_unk]() {
                        SKSE::GetTaskInterface()->AddTask([a_manager, a_actor, a_object, a_unk]() {
                            logger::trace("Hi I'm Pos Lambda Function");
                            if (a_actor) {
                                logger::trace("a_actor: {}", a_actor->GetName());
                            }
                            if (a_object) {
                                logger::trace("a_object: {}", a_object->GetName());
                            }
                            logger::trace("DrawWeaponMagicHands");
                            a_actor->DrawWeaponMagicHands(true);
                            logger::trace("Calling func");
                            RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, a_object);
                            logger::trace("Func Called ok");
                        });
                    });*/
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
                        /*Utils::UpdatePospondQueue(a_actor->formID, [a_manager, a_actor, a_object, a_unk]() {
                            SKSE::GetTaskInterface()->AddTask([a_manager, a_actor, a_object, a_unk]() {
                                logger::trace("Hi I'm Pos Lambda Function");
                                if (a_actor) {
                                    logger::trace("a_actor: {}", a_actor->GetName());
                                }
                                if (a_object) {
                                    logger::trace("a_object: {}", a_object->GetName());
                                }
                                logger::trace("DrawWeaponMagicHands");
                                a_actor->DrawWeaponMagicHands(true);
                                logger::trace("Calling func");
                                RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, a_object);
                                logger::trace("Func Called ok");
                            });
                        });*/
                        a_actor->DrawWeaponMagicHands(false);
                        actorState->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
                        return;
                    }
                }
            }
        }

        logger::trace("call func");
        return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                    a_playSounds, a_applyNow);
    }

    void UnEquipObjectHook::thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object,
                                  RE::ExtraDataList* a_extraData, std::uint32_t a_count, const RE::BGSEquipSlot* a_slot,
                                  bool a_queueEquip, bool a_forceEquip, bool a_playSounds, bool a_applyNow,
                                  const RE::BGSEquipSlot* a_slotToReplace)
    {
        if (!a_object || !a_actor || !a_manager) {
            return func(a_manager, a_actor, a_object, a_extraData, a_count, a_slot, a_queueEquip, a_forceEquip,
                        a_playSounds, a_applyNow, a_slotToReplace);
        }

        logger::trace("{} is Unequiping {}", a_actor->GetName(), a_object->GetName());

        if (a_object->IsWeapon()) {
            logger::trace("{} IsWeapon", a_object->GetName());
            if (Utils::IsInPospondQueue(a_actor->formID)) {
                logger::trace("{} is in Pospond Equip Queue not allowing to Unequip", a_actor->GetName());
                if (a_actor == RE::PlayerCharacter::GetSingleton()) {
                    /*Utils::UpdatePospondQueue(a_actor->formID, [a_manager, a_actor, a_object, a_extraData, a_unk]() {
                        SKSE::GetTaskInterface()->AddTask([a_manager, a_actor, a_object, a_extraData, a_unk]() {
                            logger::trace("Hi I'm Pos Lambda Function");
                            if (a_actor) {
                                logger::trace("a_actor: {}", a_actor->GetName());
                            }
                            if (a_object) {
                                logger::trace("a_object: {}", a_object->GetName());
                            }
                            logger::trace("DrawWeaponMagicHands");
                            a_actor->DrawWeaponMagicHands(true);
                            logger::trace("Calling func");
                            RE::ActorEquipManager::GetSingleton()->UnequipObject(a_actor, a_object);
                            logger::trace("Func Called ok");
                        });
                    });*/
                }
                return;
            }

            logger::trace("{} is NOT IsInPospondQueue", a_actor->GetName());
            if (RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
                if (actorState->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
                    RE::TESForm* RequippedObject = a_actor->GetEquippedObject(false);
                    RE::TESForm* LequippedObject = a_actor->GetEquippedObject(true);

                    if ((RequippedObject && RequippedObject->Is(RE::FormType::Weapon)) ||
                        (LequippedObject && LequippedObject->Is(RE::FormType::Weapon))) {
                        logger::trace("{} is in kDrawn Weapon State, adding to Pospond Equip Queue",
                                      a_actor->GetName());
                        /*Utils::UpdatePospondQueue(
                            a_actor->formID, [a_manager, a_actor, a_object, a_extraData, a_unk]() {
                                SKSE::GetTaskInterface()->AddTask([a_manager, a_actor, a_object, a_extraData, a_unk]() {
                                    logger::trace("Hi I'm Pos Lambda Function");
                                    if (a_actor) {
                                        logger::trace("a_actor: {}", a_actor->GetName());
                                    }
                                    if (a_object) {
                                        logger::trace("a_object: {}", a_object->GetName());
                                    }
                                    logger::trace("DrawWeaponMagicHands");
                                    a_actor->DrawWeaponMagicHands(true);
                                    logger::trace("Calling func");
                                    RE::ActorEquipManager::GetSingleton()->UnequipObject(a_actor, a_object);
                                    logger::trace("Func Called ok");
                                });
                            });*/
                        a_actor->DrawWeaponMagicHands(false);
                        actorState->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
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

        if (!Utils::IsInPospondQueue(a_actor->formID)) {
            return func(a_actor, a_delta);
        }        

        if (RE::ActorState* actorState = a_actor->AsActorState()) {  // nullptr check
            if (actorState->GetWeaponState() != RE::WEAPON_STATE::kSheathed) {
                return func(a_actor, a_delta);
            }
            std::function<void()> pospondedeFunc = Utils::GetTaskFromPospondQueue(a_actor->formID);
            logger::trace("Removing {} from PospondEquipQueue", a_actor->GetName());
            Utils::RemoveFromPospondQueue(a_actor->formID);
            pospondedeFunc();
        }
        return func(a_actor, a_delta);
    }
};
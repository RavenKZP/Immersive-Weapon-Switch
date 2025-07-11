#include "MCP.h"
#include "Settings.h"
#include "Utils.h"

namespace Helper {

    std::string WeaponStateToString(const RE::WEAPON_STATE state) {
        switch (state) {
            case RE::WEAPON_STATE::kSheathed:
                return "kSheathed";
            case RE::WEAPON_STATE::kDrawing:
                return "kDrawing";
            case RE::WEAPON_STATE::kDrawn:
                return "kDrawn";
            case RE::WEAPON_STATE::kSheathing:
                return "kSheathing";
            case RE::WEAPON_STATE::kWantToDraw:
                return "kWantToDraw";
            case RE::WEAPON_STATE::kWantToSheathe:
                return "kWantToSheathe";
            default:
                return "Unk";
        }
    }

    std::string GetSlotAsString(const RE::FormID slotID) {
        switch (slotID) {
            case 81730:
                return "Right";
            case 81731:
                return "Left";
            case 81733:
                return "Both";
            case 82408:
                return "Shield";
            default:
                return "Unk";
        }
    }

}  // Helper

namespace Utils {

    static RE::BGSKeyword* FindOrCreateKeyword(std::string kwd) {
        auto form = RE::TESForm::LookupByEditorID(kwd);
        if (form) {
            return form->As<RE::BGSKeyword>();
        }
        logger::warn("[InitGlobals]: No keyword {} found, creating it", kwd);
        const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSKeyword>();
        RE::BGSKeyword* createdkeyword = nullptr;
        if (createdkeyword = factory ? factory->Create() : nullptr; createdkeyword) {
            createdkeyword->formEditorID = kwd;
        }
        return createdkeyword;
    }

    void InitGlobals() {
#undef GetObject
        unarmed_weapon = RE::TESForm::LookupByID(0x1f4)->As<RE::TESBoundObject>();
        right_hand_slot = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
            RE::DEFAULT_OBJECT::kRightHandEquip);
        left_hand_slot = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
            RE::DEFAULT_OBJECT::kLeftHandEquip);

        switch_keyword_left = FindOrCreateKeyword("IWS_SwitchInProgressLeft");
        switch_keyword_right = FindOrCreateKeyword("IWS_SwitchInProgressRight");

        unequip_keyword_left = FindOrCreateKeyword("IWS_UnequipInProgressLeft");
        unequip_keyword_right = FindOrCreateKeyword("IWS_UnequipInProgressRight");

        equip_keyword_left = FindOrCreateKeyword("IWS_EquipInProgressLeft");
        equip_keyword_right = FindOrCreateKeyword("IWS_EquipInProgressRight");

        if (unarmed_weapon && right_hand_slot && left_hand_slot && switch_keyword_right && switch_keyword_left &&
            unequip_keyword_left && unequip_keyword_right && equip_keyword_left && equip_keyword_right) {
            logger::info("[InitGlobals]: Global variables initialized");
        } else {
            logger::error("[InitGlobals]: Error during Initialization of Global variables");
        }
    }

    void UpdateEventInfo(RE::Actor* act, RE::TESBoundObject* object, bool left, bool unequip,
                         AnimationEventSink* evSink) {
        if (!object || !act) {
            logger::error("[UpdateEventInfo]: {} is nullptr", act ? "Object" : "Actor");
            return;
        }
        logger::debug("[UpdateEventInfo]:[{} - {:08X}] {} - {} - {}", act->GetName(), act->GetFormID(),
                      object->GetName(), left ? "left" : "right", unequip ? "unequip" : "equip");
        std::unique_lock ulock(actor_equip_event_mutex);

        auto actor_curr_right = act->GetEquippedObject(false);
        auto actor_curr_left = act->GetEquippedObject(true);

        if (const auto it = actor_equip_event.find(act);
            it != actor_equip_event.end()) {  // actor already tracked, udpate event
            logger::debug("[UpdateEventInfo]:[{} - {:08X}] already tracked", act->GetName(), act->GetFormID());

            RE::TESObjectREFR::Count count = 0;

            if (!object->IsWeapon()) {
                count = 69;  // whatever more than 2 :D
            } else {
                RE::TESObjectREFR::InventoryItemMap inv = act->GetInventory();
                auto it_inv = inv.find(object);
                if (it_inv != inv.end()) {
                    count = it_inv->second.first;
                    logger::debug("[UpdateEventInfo]:[{} - {:08X}] has {} of the {}.", act->GetName(), act->GetFormID(),
                                  count, object->GetName());
                } else {
                    logger::error("[UpdateEventInfo]:[{} - {:08X}] {} not found in inventory.", act->GetName(),
                                  act->GetFormID(), object->GetName());
                    return;
                }
            }

            if (object->Is(RE::FormType::Spell) || object->Is(RE::FormType::Scroll)) {  // Double equip = both hands
                if (object == it->second.left) {
                    left = false;
                }
                if (object == it->second.right) {
                    left = true;
                }
            }

            if (object->IsWeapon() && !IsTwoHanded(object) && count > 1) {  // Double equip = both hands
                if (object == it->second.left) {
                    left = false;
                }
                if (object == it->second.right) {
                    left = true;
                }
            }

            if (left) {
                if (act->IsPlayerRef()) {
                    SetInventoryInfo(actor_curr_left, true, true);
                }
                if (!unequip) {  // always update if equip call
                    if (act->IsPlayerRef()) {
                        RemoveInventoryInfo(it->second.left);
                        SetInventoryInfo(object, true);
                    }
                    it->second.left = object;
                    it->second.unequip_left = unequip;
                    if (it->second.right == object && count < 2 && !it->second.unequip_right) {
                        it->second.right = nullptr;
                    }
                } else {
                    if (!it->second.left) {  // only update unequip if empty
                        it->second.left = object;
                        it->second.unequip_left = unequip;
                    }
                }
            } else {
                if (act->IsPlayerRef()) {
                    SetInventoryInfo(actor_curr_right, false, true);
                }
                if (!unequip) {  // always update if equip call
                    if (act->IsPlayerRef()) {
                        RemoveInventoryInfo(it->second.right);
                        SetInventoryInfo(object, false);
                    }
                    it->second.right = object;
                    it->second.unequip_right = unequip;
                    if (IsTwoHanded(object)) {
                        if (act->IsPlayerRef()) {
                            SetInventoryInfo(actor_curr_left, true, true);
                        }
                        it->second.left = nullptr;
                    }
                    if (it->second.left == object && count < 2 && !it->second.unequip_left) {
                        it->second.left = nullptr;
                    }
                } else {
                    if (!it->second.right) {  // only update unequip if empty
                        it->second.right = object;
                        it->second.unequip_right = unequip;
                    }
                }
            }
            it->second.eventSink = evSink;
            logger::debug("[UpdateEventInfo]:[{} - {:08X}] Right {}: {}", act->GetName(), act->GetFormID(),
                          it->second.unequip_right ? "unequip" : "equip",
                          it->second.right ? it->second.right->GetName() : "nullptr");
            logger::debug("[UpdateEventInfo]:[{} - {:08X}] Left {}: {}", act->GetName(), act->GetFormID(),
                          it->second.unequip_left ? "unequip" : "equip",
                          it->second.left ? it->second.left->GetName() : "nullptr");
        } else {  // actor not tracked
            logger::debug("[UpdateEventInfo]:[{} - {:08X}] not tracked", act->GetName(), act->GetFormID());
            EquipEvent temp_event;
            if (left) {
                if (act->IsPlayerRef()) {
                    SetInventoryInfo(actor_curr_left, true, true);
                    SetInventoryInfo(object, true, unequip);
                }
                temp_event.left = object;
                temp_event.unequip_left = unequip;
            } else {
                if (act->IsPlayerRef()) {
                    SetInventoryInfo(actor_curr_right, false, true);
                    SetInventoryInfo(object, false, unequip);
                    if (IsTwoHanded(object)) {
                        SetInventoryInfo(actor_curr_left, true, true);
                    }
                }
                temp_event.right = object;
                temp_event.unequip_right = unequip;
            }
            temp_event.eventSink = evSink;
            logger::debug("[UpdateEventInfo]:[{} - {:08X}] Right {}: {}", act->GetName(), act->GetFormID(),
                          temp_event.unequip_right ? "unequip" : "equip",
                          temp_event.right ? temp_event.right->GetName() : "nullptr");
            logger::debug("[UpdateEventInfo]:[{} - {:08X}] Left {}: {}", act->GetName(), act->GetFormID(),
                          temp_event.unequip_left ? "unequip" : "equip",
                          temp_event.left ? temp_event.left->GetName() : "nullptr");
            actor_equip_event.insert({act, temp_event});
        }
    }

    bool IsAlreadyTracked(RE::Actor* act) {
        std::shared_lock ulock(actor_equip_event_mutex);
        return actor_equip_event.contains(act);
    }

    void RemoveEvent(RE::Actor* act) {
        std::unique_lock ulock(actor_equip_event_mutex);
        if (const auto it = actor_equip_event.find(act); it != actor_equip_event.end()) {
            RemoveInventoryInfo(it->second.left);
            RemoveInventoryInfo(it->second.right);
            RemoveInventoryInfo(act->GetEquippedObject(false));
            RemoveInventoryInfo(act->GetEquippedObject(true));
            actor_equip_event.erase(it);
        }
    }

    void RemoveEvent(const RE::Actor* act) {
        RE::Actor* actor = ConstCastActor(act);
        RemoveEvent(actor);
    }

    void ClearAllEvents() {
        std::vector<RE::Actor*> to_remove;

        {
            std::unique_lock lock(actor_equip_event_mutex);
            for (const auto& [actor, _] : actor_equip_event) {
                to_remove.push_back(actor);
            }
        }
        for (auto* actor : to_remove) {
            RemoveEvent(actor);
            RemoveAnimationInfo(actor);
        }
    }

    EquipEvent GetEvent(RE::Actor* act) {
        std::shared_lock ulock(actor_equip_event_mutex);
        EquipEvent res{nullptr};
        if (const auto it = actor_equip_event.find(act); it != actor_equip_event.end()) {
            res = it->second;
        }
        return res;
    }

    EquipEvent GetEvent(const RE::Actor* act) {
        RE::Actor* actor = ConstCastActor(act);
        return GetEvent(actor);
    }

    AnimationEventSink* GetAnimationEventSink(RE::Actor* act) {
        AnimationEventSink* res{nullptr};
        std::shared_lock ulock(actor_equip_event_mutex);
        if (const auto it = actor_equip_event.find(act); it != actor_equip_event.end()) {
            res = it->second.eventSink;
        }
        return res;
    }

    void ExecuteEvent(const RE::Actor* const_act) {
        RE::Actor* actor = ConstCastActor(const_act);
        EquipEvent eqEve = GetEvent(const_act);  // May return empty

        Utils::RemoveEvent(actor);
        auto actor_curr_right = actor->GetEquippedObject(false);
        auto actor_curr_left = actor->GetEquippedObject(true);

        if (actor->IsPlayerRef()) {
            RemoveInventoryInfo(actor_curr_right);
            RemoveInventoryInfo(actor_curr_left);
        }

        auto EqManager = RE::ActorEquipManager::GetSingleton();
        if (eqEve.right) {
            actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kSheathed;
            if (actor->IsPlayerRef()) {
                RemoveInventoryInfo(eqEve.right);
            }
            logger::debug("[ExecuteEvent]:[{} - {:08X}] {}Equiping right: {}", actor->GetName(), actor->GetFormID(),
                          eqEve.unequip_right ? "Un" : "", eqEve.right->GetName());
            if (eqEve.right->As<RE::SpellItem>() && !eqEve.right->Is(RE::FormType::Scroll)) {
                EqManager->EquipSpell(actor, eqEve.right->As<RE::SpellItem>(), Utils::right_hand_slot);
            } else {
                if (eqEve.unequip_right) {
                    EqManager->UnequipObject(actor, eqEve.right, nullptr, 1, Utils::right_hand_slot, false, false,
                                             false, false);
                } else {
                    EqManager->EquipObject(actor, eqEve.right, nullptr, 1, Utils::right_hand_slot, false, false, false,
                                           false);
                }
            }
        }
        if (eqEve.left) {
            actor->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kSheathed;
            if (actor->IsPlayerRef()) {
                RemoveInventoryInfo(eqEve.left);
            }
            logger::debug("[ExecuteEvent]:[{} - {:08X}] {}Equiping left: {}", actor->GetName(), actor->GetFormID(),
                          eqEve.unequip_left ? "Un" : "", eqEve.left->GetName());

            if (eqEve.left->As<RE::SpellItem>() && !eqEve.left->Is(RE::FormType::Scroll)) {
                EqManager->EquipSpell(actor, eqEve.left->As<RE::SpellItem>(), Utils::left_hand_slot);
            } else {
                if (IsLeftOnly(eqEve.left)) {
                    if (eqEve.unequip_left) {
                        EqManager->UnequipObject(actor, eqEve.left, nullptr, 1, nullptr, false, false, false, false);
                    } else {
                        if (eqEve.left->Is(RE::FormType::Armor)) {
                            EqManager->EquipObject(actor, eqEve.left, nullptr, 1,
                                                   eqEve.left->As<RE::TESObjectARMO>()->GetEquipSlot(), false, false,
                                                   false, false);
                            RE::SendUIMessage::SendInventoryUpdateMessage(actor, nullptr);

                        } else if (eqEve.left->Is(RE::FormType::Light)) {
                            EqManager->EquipObject(actor, eqEve.left, nullptr, 1,
                                                   eqEve.left->As<RE::TESObjectLIGH>()->GetEquipSlot(), false, false,
                                                   false, false);
                        } else {
                            EqManager->EquipObject(actor, eqEve.left, nullptr, 1, nullptr, false, false, false, false);
                        }
                    }
                } else {
                    if (eqEve.unequip_left) {
                        EqManager->UnequipObject(actor, eqEve.left, nullptr, 1, Utils::left_hand_slot, false, false,
                                                 false, false);
                    } else {
                        EqManager->EquipObject(actor, eqEve.left, nullptr, 1, Utils::left_hand_slot, false, false,
                                               false, false);
                    }
                }
            }
        }
        actor->DrawWeaponMagicHands(true);
        RE::SendUIMessage::SendInventoryUpdateMessage(actor, nullptr);
    }

    bool IsInHand(RE::TESBoundObject* a_object) {
        if (a_object == nullptr) {  // Empty hand
            return true;
        }
        if (a_object == unarmed_weapon) {  // brawl
            return true;
        }
        if (a_object->Is(RE::FormType::Weapon) || a_object->Is(RE::FormType::Light) ||
            a_object->Is(RE::FormType::Scroll)) {
            return true;
        }
        if (a_object->Is(RE::FormType::Spell)) {
            if (a_object->As<RE::SpellItem>()->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                return true;
            }
        }
        if (a_object->Is(RE::FormType::Armor)) {
            if (const auto equip_slot = a_object->As<RE::TESObjectARMO>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == EquipSlots::Shield) {
                return true;
            }
        }
        return false;
    }

    bool IsWhitelistUnequip(RE::TESBoundObject* a_object) {
        bool res = false;
        if (a_object->Is(RE::FormType::Light) || a_object->Is(RE::FormType::Scroll) ||
            a_object->Is(RE::FormType::Spell)) {
            res = true;
        }
        if (a_object->IsWeapon() && a_object->As<RE::TESObjectWEAP>()->IsBound()) {
            res = true;
        }
        return res;
    }

    bool IsTwoHanded(RE::TESForm* a_object) {
        bool res = false;
        if (a_object && a_object->IsWeapon()) {
            if (const auto equip_slot = a_object->As<RE::TESObjectWEAP>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == Utils::EquipSlots::Both) {
                res = true;
            }
        }
        if (a_object && a_object->Is(RE::FormType::Scroll)) {
            if (const auto equip_slot = a_object->As<RE::ScrollItem>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == Utils::EquipSlots::Both) {
                res = true;
            }
        }
        if (a_object && a_object->Is(RE::FormType::Spell)) {
            if (const auto equip_slot = a_object->As<RE::SpellItem>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == Utils::EquipSlots::Both) {
                res = true;
            }
        }
        return res;
    }

    bool IsTwoHanded(RE::TESBoundObject* a_object) {
        bool res = false;
        if (a_object && a_object->IsWeapon()) {
            if (const auto equip_slot = a_object->As<RE::TESObjectWEAP>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == Utils::EquipSlots::Both) {
                res = true;
            }
        }
        if (a_object && a_object->Is(RE::FormType::Scroll)) {
            if (const auto equip_slot = a_object->As<RE::ScrollItem>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == Utils::EquipSlots::Both) {
                res = true;
            }
        }
        if (a_object && a_object->Is(RE::FormType::Spell)) {
            if (const auto equip_slot = a_object->As<RE::SpellItem>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == Utils::EquipSlots::Both) {
                res = true;
            }
        }
        logger::debug("[IsTwoHanded]:[{} - {:08X}] {}", a_object->GetName(),
                      a_object->GetFormID(), res);
        return res;
    }

    bool IsLeftOnly(RE::TESBoundObject* a_object) {
        bool res = false;
        if (a_object->Is(RE::FormType::Armor)) {
            if (const auto equip_slot = a_object->As<RE::TESObjectARMO>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == Utils::EquipSlots::Shield) {
                res = true;
            }
        }
        if (a_object->Is(RE::FormType::Light)) {
            res = true;
        }
        if (a_object->IsWeapon()) {
            if (a_object->As<RE::TESObjectWEAP>()->GetEquipSlot() == Utils::left_hand_slot) {
                res = true;
            }
        }
        return res;
    }

    std::pair<bool, bool> GetIfHandsEmpty(RE::Actor* act) {
        auto actor_right_hand = act->GetEquippedObject(false);
        auto actor_left_hand = act->GetEquippedObject(true);

        logger::debug("[GetIfHandsEmpty]:[{} - {:08X}] Right: {} Left: {}", act->GetName(), act->GetFormID(),
                      actor_right_hand ? actor_right_hand->GetName() : "nothing",
                      actor_left_hand ? actor_left_hand->GetName() : "nothing");

        bool left_empty = false;
        bool right_empty = false;

        if (!actor_left_hand) {
            left_empty = true;
        } else if (actor_left_hand->Is(RE::FormType::Spell) || actor_left_hand->Is(RE::FormType::Light) ||
                   actor_left_hand->Is(RE::FormType::Scroll)) {
            left_empty = true;
        } else if (actor_left_hand->IsWeapon() && actor_left_hand->As<RE::TESObjectWEAP>()->IsBound()) {
            left_empty = true;
        }

        if (!actor_right_hand) {
            right_empty = true;
        } else if (actor_right_hand->Is(RE::FormType::Spell) || actor_right_hand->Is(RE::FormType::Scroll)) {
            right_empty = true;
        } else if (actor_right_hand->IsWeapon() && actor_right_hand->As<RE::TESObjectWEAP>()->IsBound()) {
            right_empty = true;
        }

        return std::make_pair(right_empty, left_empty);
    }

    template <typename T>
    void SetInventoryInfo(T* obj, bool left, bool unequip) {
        if (!obj) {
            return;
        }
        RE::BGSKeywordForm* kwdForm = obj->As<RE::BGSKeywordForm>();
        if (unequip) {
            if (left) {
                if (kwdForm && unequip_keyword_left) {
                    if (!kwdForm->HasKeyword(unequip_keyword_left)) {
                        kwdForm->AddKeyword(unequip_keyword_left);
                    }
                }
            } else {
                if (kwdForm && unequip_keyword_right) {
                    if (!kwdForm->HasKeyword(unequip_keyword_right)) {
                        kwdForm->AddKeyword(unequip_keyword_right);
                    }
                }
            }
        } else {
            if (left) {
                if (kwdForm && switch_keyword_left) {
                    if (!kwdForm->HasKeyword(switch_keyword_left)) {
                        kwdForm->AddKeyword(switch_keyword_left);
                    }
                }
            } else {
                if (kwdForm && switch_keyword_right) {
                    if (!kwdForm->HasKeyword(switch_keyword_right)) {
                        kwdForm->AddKeyword(switch_keyword_right);
                    }
                }
            }
        }
        RE::SendUIMessage::SendInventoryUpdateMessage(RE::PlayerCharacter::GetSingleton(), nullptr);
    }

    template <typename T>
    void RemoveInventoryInfo(T* obj) {
        if (!obj) {
            return;
        }
        RE::BGSKeywordForm* kwdForm = obj->As<RE::BGSKeywordForm>();
        if (kwdForm && switch_keyword_left) {
            if (kwdForm->HasKeyword(switch_keyword_left)) {
                kwdForm->RemoveKeyword(switch_keyword_left);
            }
        }
        if (kwdForm && switch_keyword_right) {
            if (kwdForm->HasKeyword(switch_keyword_right)) {
                kwdForm->RemoveKeyword(switch_keyword_right);
            }
        }
        if (kwdForm && unequip_keyword_left) {
            if (kwdForm->HasKeyword(unequip_keyword_left)) {
                kwdForm->RemoveKeyword(unequip_keyword_left);
            }
        }
        if (kwdForm && unequip_keyword_right) {
            if (kwdForm->HasKeyword(unequip_keyword_right)) {
                kwdForm->RemoveKeyword(unequip_keyword_right);
            }
        }
        RE::SendUIMessage::SendInventoryUpdateMessage(RE::PlayerCharacter::GetSingleton(), nullptr);
    }

    static bool HasActiveEffectsByEffect(RE::MagicTarget* target, RE::EffectSetting* effectSetting) {
        if (!target || !effectSetting) {
            return false;
        }

        const auto activeEffects = target->GetActiveEffectList();
        if (!activeEffects) {
            return false;
        }

        bool haseffect = false;
        for (auto* effect : *activeEffects) {
            if (effect && effect->effect && effect->effect->baseEffect == effectSetting) {
                haseffect = true;
            }
        }
        return haseffect;
    }

    void SetAnimationInfo(RE::Actor* act, bool left, bool equip) {
        if (!act) {
            return;
        }

        logger::debug("[SetAnimationInfo]:[{} - {:08X}] left {} equip {}", act->GetName(), act->GetFormID(), left,
                      equip);
        if (left) {
            if (equip) {
                act->SetGraphVariableBool("bIWS_EquipLeft", true);
            } else {
                act->SetGraphVariableBool("bIWS_UnequipLeft", true);
            }
        } else {
            if (equip) {
                act->SetGraphVariableBool("bIWS_EquipRight", true);
            } else {
                act->SetGraphVariableBool("bIWS_UnequipRight", true);
            }
        }
    }

    void SetAnimationInfoHandSwitch(RE::Actor* act) {
        if (!act) {
            return;
        }
        logger::debug("[SetAnimationInfoHandSwitch]:[{} - {:08X}]", act->GetName(), act->GetFormID());

        act->SetGraphVariableBool("bIWS_Switch", true);
    }

    void ProceedAnimationInfo(RE::Actor* act) {
        if (!act) {
            return;
        }

        logger::debug("[ProceedAnimationInfo]:[{} - {:08X}]", act->GetName(), act->GetFormID());

        bool LeftGraphVariable;
        if (act->GetGraphVariableBool("bIWS_UnequipLeft", LeftGraphVariable) && LeftGraphVariable) {
            act->SetGraphVariableBool("bIWS_EquipLeft", true);
            act->SetGraphVariableBool("bIWS_UnequipLeft", false);
        }

        bool RightGraphVariable;
        if (act->GetGraphVariableBool("bIWS_UnequipRight", RightGraphVariable) && RightGraphVariable) {
            act->SetGraphVariableBool("bIWS_EquipRight", true);
            act->SetGraphVariableBool("bIWS_UnequipRight", false);
        } 
    }

    void RemoveAnimationInfo(RE::Actor* act) {
        if (!act) {
            return;
        }

        logger::debug("[RemoveAnimationInfo]:[{} - {:08X}]", act->GetName(), act->GetFormID());

        act->SetGraphVariableBool("bIWS_EquipLeft", false);
        act->SetGraphVariableBool("bIWS_UnequipLeft", false);
        act->SetGraphVariableBool("bIWS_EquipRight", false);
        act->SetGraphVariableBool("bIWS_UnequipRight", false);
        act->SetGraphVariableBool("bIWS_Switch", false);
    }

}  // Utils

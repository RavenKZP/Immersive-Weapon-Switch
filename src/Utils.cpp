#include "MCP.h"
#include "Settings.h"
#include "Utils.h"

#include "IconsFontAwesome6.h"

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

    void InitGlobals() {
#undef GetObject
        unarmed_weapon = RE::TESForm::LookupByID(0x1f4)->As<RE::TESBoundObject>();
        right_hand_slot = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
            RE::DEFAULT_OBJECT::kRightHandEquip);
        left_hand_slot = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
            RE::DEFAULT_OBJECT::kLeftHandEquip);

        // Needed for timeout calculation
        gameHour = RE::TESForm::LookupByEditorID("gamehour")->As<RE::TESGlobal>();
        timescale = RE::TESForm::LookupByEditorID("timescale")->As<RE::TESGlobal>();

        if (unarmed_weapon && right_hand_slot && left_hand_slot && gameHour && timescale) {
            logger::info("Global variables initialized");
        } else {
            logger::error("Error during Initialization of Global variables");
        }
    }

    void UpdateQueue(RE::FormID actID, const EquipEvent& equipdata) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actID);
            it != actor_queue.end()) {  // actor already tracked, add next event to the queue
            it->second.queue.push(equipdata);
            if (it->second.last_object != equipdata.object) {
                it->second.timestamp = gameHour->value;
                it->second.last_object = equipdata.object;
            }
        } else {  // actor not tracked
            std::queue<EquipEvent> temp_queue;
            temp_queue.push(equipdata);
            actor_queue.insert({actID, EquipQueueData{temp_queue, equipdata.object, gameHour->value}});
        }
    }

    bool IsInQueue(const RE::FormID actID) {
        std::shared_lock ulock(actor_queue_mutex);
        return actor_queue.contains(actID);
    }

    void RemoveFromQueue(const RE::FormID actID) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            actor_queue.erase(it);
        }
    }

    void ClearQueue() {
        std::unique_lock ulock(actor_queue_mutex);
        actor_queue.clear();
    }

    std::queue<EquipEvent> GetQueue(const RE::FormID actID) {
        std::shared_lock ulock(actor_queue_mutex);
        std::queue<EquipEvent> res;
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            res = it->second.queue;
        }
        return res;
    }

    void UpdateTimestamp(RE::FormID actID) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            it->second.timestamp = gameHour->value;
        }
    }

    float GetTimestamp(RE::FormID actID) {
        std::shared_lock ulock(actor_queue_mutex);
        float res = 0.0f;
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            res = it->second.timestamp;
        }
        return res;
    }

    RE::TESBoundObject* GetLastObject(RE::FormID actID) {
        std::shared_lock ulock(actor_queue_mutex);
        RE::TESBoundObject* res = nullptr;
        if (const auto it = actor_queue.find(actID); it != actor_queue.end()) {
            res = it->second.last_object;
        }
        return res;
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
        if (a_object->Is(RE::FormType::Armor)) {
            if (const auto equip_slot = a_object->As<RE::TESObjectARMO>()->GetEquipSlot();
                equip_slot && equip_slot->GetFormID() == EquipSlots::Shield) {
                return true;
            }
        }
        return false;
    }

    bool IsHandFree(RE::FormID slotID, RE::Actor* actor, RE::TESBoundObject* a_object) {
        bool right_empty = false;
        bool left_empty = false;
        const RE::TESForm* RightHandObj = actor->GetEquippedObject(false);
        const RE::TESForm* LeftHandObj = actor->GetEquippedObject(true);

        logger::trace("IsHandFree Obj {} RightObj {} LeftObj {}", a_object ? a_object->GetName() : "nullptr",
                      RightHandObj ? RightHandObj->GetName() : "nullptr",
                      LeftHandObj ? LeftHandObj->GetName() : "nullptr");

        if (a_object && ((a_object == RightHandObj) || (a_object == LeftHandObj))) {
            logger::trace("it's the same object");
            return true;
        }

        if (LeftHandObj == nullptr) {
            left_empty = true;
        } else if (LeftHandObj == unarmed_weapon) {
            left_empty = true;
        } else if (LeftHandObj->Is(RE::FormType::Spell)) {
            left_empty = true;
        } else if (LeftHandObj->Is(RE::FormType::Scroll)) {
            left_empty = true;
        } else if (LeftHandObj->Is(RE::FormType::Light)) {
            left_empty = true;
        } else if (LeftHandObj->IsWeapon() && LeftHandObj->As<RE::TESObjectWEAP>()->IsBound()) {
            left_empty = true;
        }

        if (RightHandObj == nullptr) {
            right_empty = true;
        } else if (RightHandObj == unarmed_weapon) {
            right_empty = true;
        } else if (RightHandObj->Is(RE::FormType::Spell)) {
            right_empty = true;
        } else if (RightHandObj->Is(RE::FormType::Scroll)) {
            right_empty = true;
        } else if (RightHandObj->IsWeapon() && RightHandObj->As<RE::TESObjectWEAP>()->IsBound()) {
            right_empty = true;
        }

        //For Bowe can Crossbows Requested slot is Right, when they took both hands
        if (a_object) {
            if ((a_object->IsWeapon()) &&
                ((a_object->As<RE::TESObjectWEAP>()->IsBow()) || (a_object->As<RE::TESObjectWEAP>()->IsCrossbow()))) {
                slotID = EquipSlots::Both;
            }
        }

        logger::trace("IsHandFree Right {} Left {}, Requesetd {}", right_empty, left_empty,
                      Helper::GetSlotAsString(slotID));

        switch (slotID) {
            case EquipSlots::Left:
            case EquipSlots::Shield: {
                return left_empty;
            }
            case EquipSlots::Right: {
                return right_empty;
            }
            case EquipSlots::Both: {
                return (right_empty && left_empty);
            }
            default: {
                return false;
            }
        }
    }

    EquipSlots SetEquipSlot(RE::TESBoundObject* a_object, RE::Actor* a_actor) {
        EquipSlots slot = EquipSlots::Right;
        if (a_object->IsArmor()) {
            slot = EquipSlots::Shield;
        } else if (a_object->Is(RE::FormType::Light)) {
            slot = EquipSlots::Left;
        } else if (a_object->IsWeapon()) {
            switch (a_object->As<RE::TESObjectWEAP>()->GetWeaponType()) {
                case (RE::WeaponTypes::WEAPON_TYPE::kTwoHandAxe):
                case (RE::WeaponTypes::WEAPON_TYPE::kTwoHandSword):
                case (RE::WeaponTypes::WEAPON_TYPE::kBow):
                case (RE::WeaponTypes::WEAPON_TYPE::kCrossbow): {
                    slot = EquipSlots::Both;
                    break;
                }
                default: {
                    const RE::TESForm* RightHandObj = a_actor->GetEquippedObject(false);
                    if (IsHandFree(EquipSlots::Right, a_actor, a_object)) {
                        slot = EquipSlots::Right;
                    } else if (IsHandFree(EquipSlots::Left, a_actor, a_object)) {
                        slot = EquipSlots::Left;
                    } else if (RightHandObj == a_object) {
                        slot = EquipSlots::Left;
                    }
                    break;
                }
            }
        }
        return slot;
    }

    bool DropIfLowHP(RE::Actor* a_actor) {
        bool res = false;
        auto Settings = Settings::GetSingleton();
        if ((a_actor->IsPlayerRef() && Settings->PC_Drop_Weapons) ||
            (!a_actor->IsPlayerRef() && Settings->NPC_Drop_Weapons)) {
            const float curr_hp = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
            const float max_hp = a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHealth);
            logger::trace("{} / {} hp", curr_hp, max_hp);
            if (max_hp != 0.0) {
                if (((a_actor->IsPlayerRef()) && (curr_hp / max_hp) * 100.0f <= Settings->PC_Health_Drop) ||
                    ((!a_actor->IsPlayerRef()) && (curr_hp / max_hp) * 100.0f <= Settings->NPC_Health_Drop)) {
                    RE::TESForm* Left = a_actor->GetEquippedObject(true);
                    RE::TESForm* Right = a_actor->GetEquippedObject(false);
                    if (Left == Right) {
                        logger::trace("{} dropping {}", a_actor->GetName(), Left->GetName());
                        RE::TESBoundObject* bound = Left->As<RE::TESBoundObject>();
                        a_actor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
                        return true;
                    }
                    if (Right) {
                        logger::trace("{} dropping {}", a_actor->GetName(), Right->GetName());
                        RE::TESBoundObject* bound = Right->As<RE::TESBoundObject>();
                        a_actor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
                        res = true;
                    }
                    if (Left) {
                        logger::trace("{} dropping {}", a_actor->GetName(), Left->GetName());
                        RE::TESBoundObject* bound = Left->As<RE::TESBoundObject>();
                        a_actor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
                        res = true;
                    }
                }
            }
        }
        return res;
    }

    //WIP - I would like to get Inventory Icon from given object to use it in the display
    bool GetIconForObject(RE::TESBoundObject* obj, RE::GFxValue& outIcon) {
        if (!obj) {
            logger::trace("[GetIconForObject] ERROR: Null TESBoundObject passed.");
            return false;
        }
        auto player = RE::PlayerCharacter::GetSingleton();
        auto inv = player->GetInventory();

        if (obj->As<RE::TESObjectWEAP>()) {
            logger::trace("{}", obj->As<RE::TESObjectWEAP>()->icon.textureName);
        }
        
        auto invObj = inv.find(obj);
        return true;
    }

    //Temp Solution until GetIconForObject is not implemented
    void SetFaIcons(RE::Actor* actor, RE::TESBoundObject* object, RE::FormID slotID, RE::SpellItem* spell) {
        auto Right = actor->GetEquippedObject(false);
        auto Left = actor->GetEquippedObject(false);

        if (Right) {
            Utils::OldObj1 = Right->GetName();
            if (Right->IsWeapon()) {
                Utils::OldObj1 = "\xef\x9c\x9c";
            } else {
                Utils::OldObj1 = ICON_FA_HAND_SPARKLES;
            }
        } else {
            Utils::OldObj1 = ICON_FA_HAND_FIST;
        }
        if (Left) {
            Utils::OldObj2 = Left->GetName();
            if (Left->IsArmor()) {
                Utils::OldObj2 = ICON_FA_SHIELD;
            } else if (Left->IsWeapon()) {
                Utils::OldObj2 = "\xef\x9c\x9c";
            } else {
                Utils::OldObj2 = ICON_FA_HAND_SPARKLES;
            }
        } else {
            Utils::OldObj2 = ICON_FA_HAND_FIST;
        }

        if (object) {
            std::string NameOrIcon = object->GetName();
            if (object->IsArmor()) {
                NameOrIcon = ICON_FA_SHIELD;
            } else if (object->IsWeapon()) {
                NameOrIcon = "\xef\x9c\x9c";
            } else {
                NameOrIcon = ICON_FA_FIRE;
            }

            if (slotID == EquipSlots::Both) {
                Utils::NewObj1 = NameOrIcon;
                Utils::NewObj2 = "";
            }
            if (slotID == EquipSlots::Right) {
                Utils::NewObj1 = NameOrIcon;
            }
            if (slotID == EquipSlots::Left) {
                Utils::NewObj2 = NameOrIcon;
            }
            if (slotID == EquipSlots::Shield) {
                Utils::NewObj2 = NameOrIcon;
            }
        } else if (spell) {
            if (slotID == EquipSlots::Right) {
                Utils::NewObj1 = ICON_FA_HAND_SPARKLES;
            }
            if (slotID == EquipSlots::Left) {
                Utils::NewObj2 = ICON_FA_HAND_SPARKLES;
            }
        } else {
            if (slotID == EquipSlots::Right) {
                Utils::NewObj1 = ICON_FA_HAND_FIST;
            }
            if (slotID == EquipSlots::Left) {
                Utils::NewObj2 = ICON_FA_HAND_FIST;
            }
        }
    }

    void UpdateEquipingInfo(RE::Actor* actor, RE::TESBoundObject* object, RE::FormID slotID, RE::SpellItem* spell) {
        if (!actor->IsPlayerRef()) {
            return;
        }
        //SetFaIcons(actor,object,slotID,spell);
        auto keywordForm = object->As<RE::BGSKeywordForm>();
        RE::BGSKeyword* keyword = nullptr;
        const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSKeyword>();
        if (keyword = factory ? factory->Create() : nullptr; keyword) {
            keyword->formEditorID = "ABC_I4WIO_Chakram2H";
        }

        if (keywordForm && keyword) {
            logger::trace("keyword found");
            keywordForm->AddKeyword(keyword->As<RE::BGSKeyword>());
        }
        ShowSwitchIcons = true;
        RE::SendUIMessage::SendInventoryUpdateMessage(actor, nullptr);
    }

    void RemoveEquipingInfo(RE::Actor* actor) {
        if (!actor->IsPlayerRef()) {
            return;
        }
        ShowSwitchIcons = false;
        RE::SendUIMessage::SendInventoryUpdateMessage(actor, nullptr);
    }
}

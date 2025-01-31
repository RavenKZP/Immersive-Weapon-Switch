#include "MCP.h"
#include "Settings.h"
#include "Utils.h"

namespace Helper {

    std::string WeaponStateToString(const RE::WEAPON_STATE state) {
        std::string res = "Unk";
        if (state == RE::WEAPON_STATE::kSheathed) {
            res = "kSheathed";
        }
        if (state == RE::WEAPON_STATE::kDrawing) {
            res = "kDrawing";
        }
        if (state == RE::WEAPON_STATE::kDrawn) {
            res = "kDrawn";
        }
        if (state == RE::WEAPON_STATE::kSheathing) {
            res = "kSheathing";
        }
        if (state == RE::WEAPON_STATE::kWantToDraw) {
            res = "kWantToDraw";
        }
        if (state == RE::WEAPON_STATE::kWantToSheathe) {
            res = "kWantToSheathe";
        }
        return res;
    }

    std::string GetSlotAsString(const RE::FormID slotID) { 
        std::string res = "Unk";
        switch (slotID) {
            case (81730):
                res = "Right";
                break;
            case (81731):
                res = "Left";
                break;
            case (81733):
                res = "Both";
                break;
            case (82408):
                res = "Shield";
                break;
            default: 
                break;
        }
        return res;
    }
}

namespace Utils {

    void InitGlobals() {
#undef GetObject
        unarmed_weapon = RE::TESForm::LookupByID(0x1f4)->As<RE::TESBoundObject>();
        right_hand_slot = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
            RE::DEFAULT_OBJECT::kRightHandEquip);
        left_hand_slot = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSEquipSlot>(
            RE::DEFAULT_OBJECT::kLeftHandEquip);
        gameHour = RE::TESForm::LookupByEditorID("gamehour")->As<RE::TESGlobal>();
        timescale = RE::TESForm::LookupByEditorID("timescale")->As<RE::TESGlobal>();
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

        if (a_object && ((a_object == RightHandObj) || (a_object == LeftHandObj))) {
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
            logger::trace("{} IsMagicItem", LeftHandObj->GetName());
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
            logger::trace("{} IsMagicItem", RightHandObj->GetName());
            right_empty = true;
        }

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
        }
        return false;
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
                    const RE::TESForm* LeftHandObj = a_actor->GetEquippedObject(true);
                    if (IsHandFree(EquipSlots::Right, a_actor)) {
                        slot = EquipSlots::Right;
                    } else if (IsHandFree(EquipSlots::Left, a_actor)) {
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
        if ((a_actor->IsPlayerRef() && Settings::PC_Drop_Weapons) ||
            (!a_actor->IsPlayerRef() && Settings::NPC_Drop_Weapons)) {
            const float curr_hp = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
            const float max_hp = a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHealth);
            logger::trace("{} / {} hp", curr_hp, max_hp);
            if (max_hp != 0.0) {
                if (((a_actor->IsPlayerRef()) && (curr_hp / max_hp) * 100.0f <= Settings::PC_Health_Drop) ||
                    ((!a_actor->IsPlayerRef()) && (curr_hp / max_hp) * 100.0f <= Settings::NPC_Health_Drop)) {
                    RE::TESForm* Left = a_actor->GetEquippedObject(true);
                    RE::TESForm* Right = a_actor->GetEquippedObject(false);
                    if (Left == Right) {
                        logger::trace("{} dropping {}", a_actor->GetName(), Left->GetName());
                        RE::TESBoundObject* bound = Left->As<RE::TESBoundObject>();
                        a_actor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr);
                        res = true;
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

    void UpdateEquipingInfo(RE::TESBoundObject* a_object, RE::ExtraDataList* a_extraData) {
        if (!a_object) {
            return;
        }
        /*
        RE::TESFullName* ObjName = a_object->As<RE::TESFullName>();
        if (ObjName) {
            std::string Name = ObjName->GetFullName();
            std::string suffix = " (Equipping)";

            if (Name.find(suffix) == std::string::npos) {
                Name += suffix;
                ObjName->SetFullName(Name.c_str());
            }
        }*/        
        //RE::InventoryEntryData;
        RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
        RE::ExtraWorn* MyExtraWorn = RE::BSExtraData::Create<RE::ExtraWorn>();

        auto inv = player->GetInventory();

        auto it = inv.find(a_object);
        auto& [count, entry] = it->second;
        auto xList = entry->extraLists->front();
        if (xList) {
            logger::trace("Adding MyExtraWorn...");
            xList->Add(MyExtraWorn);
        }

        logger::trace("All OK Update Inventory");
        RE::SendUIMessage::SendInventoryUpdateMessage(player, nullptr);
        logger::trace("Inventory Updated");
    }

    void RemoveEquipingInfo(RE::TESBoundObject* a_object) {
        if (!a_object) {
            return;
        }
        /*
        RE::TESFullName* ObjName = a_object->As<RE::TESFullName>();
        if (ObjName) {
            std::string Name = ObjName->GetFullName();
            std::string suffix = " (Equipping)";

            if (Name.size() >= suffix.size() && Name.compare(Name.size() - suffix.size(), suffix.size(), suffix) == 0) {
                Name.erase(Name.size() - suffix.size());
                ObjName->SetFullName(Name.c_str());
            }
        */
        RE::SendUIMessage::SendInventoryUpdateMessage(RE::PlayerCharacter::GetSingleton(), nullptr);
    }
}
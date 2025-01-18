#include "Utils.h"

namespace Helper {

    std::string WeaponStateToString(RE::WEAPON_STATE state) {
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
}

namespace Utils {

    void UpdatePospondEquipQueue(RE::Actor* a_actor, RE::TESBoundObject* a_object, bool unqeuip_flag, bool left_hand) {
        std::unique_lock ulock(actor_queue_mutex);
        auto actor_info = ActorInfo(a_actor, a_object, unqeuip_flag, left_hand);
#ifndef NDEBUG
        logger::trace("Add / Update {} with {} to PospondEquipQueue", a_actor->GetName(),
                      unqeuip_flag ? "Nothing" : a_object->GetName());
#endif
        if (const auto it = actor_queue.find(actor_info); it != actor_queue.end()) {
            actor_queue.erase(it);
        }
            actor_queue.insert(actor_info);
    }

    bool IsInPospondEquipQueue(ActorInfo actInf) {
        std::unique_lock ulock(actor_queue_mutex);
        bool res = false;
        if (actor_queue.contains(actInf)) {
            res = true;
        }
        return res;
    }

    void RemoveFromPospondEquipQueue(ActorInfo actInf) {
        std::unique_lock ulock(actor_queue_mutex);
        if (const auto it = actor_queue.find(actInf); it != actor_queue.end()) {
#ifndef NDEBUG
        logger::trace("Remove ID: {} with {} from PospondEquipQueue", it->baseID,
                              it->unequip ? "Nothing" : it->queued_weap->GetName());
#endif
            actor_queue.erase(it);
        }
    }

    void ClearPospondEquipQueue() {
        std::unique_lock ulock(actor_queue_mutex);
        actor_queue.clear();
    }

    RE::TESBoundObject* GetObjectFromPospondEquipQueue(ActorInfo actInf) {
        std::unique_lock ulock(actor_queue_mutex);
        RE::TESBoundObject* res = nullptr;
        if (const auto it = actor_queue.find(actInf); it != actor_queue.end()) {
            res = it->queued_weap;
        }
        return res;
    }

    
    bool GetUnequipFromPospondEquipQueue(ActorInfo actInf) {
        std::unique_lock ulock(actor_queue_mutex);
        bool res = false;
        if (const auto it = actor_queue.find(actInf); it != actor_queue.end()) {
            res = it->unequip;
        }
        return res;
    }
}
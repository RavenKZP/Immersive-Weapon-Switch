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

    void UpdatePospondQueue(const RefID id, std::function<void()> task) {  // Make it FIFO
        std::unique_lock ulock(queue_mutex);
        if (const auto it = pospond_queue.find(id); it != pospond_queue.end()) {
            it->second = task;
        } else {
            pospond_queue.insert({id, task});
        }
#ifndef NDEBUG
        logger::trace("Add ID: {} to Pospond Queue", id);
#endif
    }

    bool IsInPospondQueue(const RefID id) {
        std::unique_lock ulock(queue_mutex);
        bool res = false;
        if (pospond_queue.contains(id)) {
            res = true;
        }
        return res;
    }

    void RemoveFromPospondQueue(const RefID id) {
        std::unique_lock ulock(queue_mutex);
        if (const auto it = pospond_queue.find(id); it != pospond_queue.end()) {
#ifndef NDEBUG
            logger::trace("Remove ID: {} from Pospond Queue", id);
#endif
            pospond_queue.erase(it);
        }
    }

    void ClearPospondQueue() {
        std::unique_lock ulock(queue_mutex);
        pospond_queue.clear();
    }

    
    std::function<void()> GetTaskFromPospondQueue(const RefID id) {
        std::unique_lock ulock(queue_mutex);
        if (const auto it = pospond_queue.find(id); it != pospond_queue.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    bool IsInHand(RE::TESBoundObject* a_object) {
        bool res = false;
        if (a_object->IsWeapon()) { // All types of weapons (Mele, Bows, CrossBows, Staff)
            res = true;
        } else if (a_object->IsArmor()) { // Improve detection it should be onlt for Shields
            res = true;
        } else if (a_object->Is(RE::FormType::Scroll)) {
            res = true;
        } else if (a_object->Is(RE::FormType::Spell)) {
            res = true;
        } else if (a_object->Is(RE::FormType::Light)) { // Torch, Lantern
            res = true;
        }
        return res;
    }
}
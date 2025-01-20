#ifndef UTILS_H
#define UTILS_H

#include <functional>
#include <shared_mutex>
#include<unordered_set>

namespace Helper {

    std::string WeaponStateToString(RE::WEAPON_STATE state);
}

namespace Utils {

    void UpdatePospondQueue(const RefID id, std::function<void()> event);
    bool IsInPospondQueue(const RefID id);
    void RemoveFromPospondQueue(const RefID id);
    std::function<void()> GetTaskFromPospondQueue(const RefID id);
    void ClearPospondQueue();

    bool IsInHand(RE::TESBoundObject* a_object);

    inline std::shared_mutex queue_mutex;
    inline std::unordered_map<RefID, std::function<void()>> pospond_queue;
}

#endif  // UTILS_H
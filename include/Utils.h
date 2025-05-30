#pragma once
#include <shared_mutex>
#include <unordered_set>

#include "Animation.h"

namespace Helper {

    std::string WeaponStateToString(RE::WEAPON_STATE state);
    std::string GetSlotAsString(RE::FormID slotID);
}

namespace Utils {

    const enum EquipSlots { Shield = 82408, Right = 81730, Left = 81731, Both = 81733 };

    struct EquipEvent {
        RE::TESBoundObject* right = nullptr;
        RE::TESBoundObject* left = nullptr;
        bool unequip_right = false;
        bool unequip_left = false;
        AnimationEventSink* eventSink;
    };

    void InitGlobals();

    inline RE::Actor* GetActor(const RE::Actor* act) { return const_cast<RE::Actor*>(act); };

    // Equip Event Related Functions
    void UpdateEventInfo(RE::Actor* actID, RE::TESBoundObject* object, bool left, bool unequip, AnimationEventSink* evSink);
    bool IsAlreadyTracked(RE::Actor* actID);
    void RemoveEvent(RE::Actor* actID);
    void RemoveEvent(const RE::Actor* actID);
    void ClearAllEvents();
    EquipEvent GetEvent(RE::Actor* actID);
    EquipEvent GetEvent(const RE::Actor* act);
    AnimationEventSink* GetAnimationEventSink(RE::Actor* act);
    void ExecuteEvent(const RE::Actor* act);

    // Returns true whenever given object is equipable in left, right or both hands
    bool IsInHand(RE::TESBoundObject* a_object);
    // Returns true whenever object don't have unequip animation (e.g. Lights)
    bool IsWhitelistUnequip(RE::TESBoundObject* a_object);
    // Returns True whenever given object is two handed weapon
    bool IsTwoHanded(RE::TESBoundObject* a_object);
    bool IsTwoHanded(RE::TESForm* a_object);
    // Returns True whenever given object is only left hand
    bool IsLeftOnly(RE::TESBoundObject* a_object);
    // Returns pair for right and left hand containing if hand is empty
    std::pair<bool, bool> GetIfHandsEmpty(RE::Actor* act);

    //I4 Icons
    template <typename T>
    void SetInventoryInfo(T* obj, bool left, bool unequip = false);
    template <typename T>
    void RemoveInventoryInfo(T* obj);

    // OAR Animations
    void SetAnimationInfo(RE::Actor* act, bool left, bool equip = true);
    void ProceedAnimationInfo(RE::Actor* act);
    void RemoveAnimationInfo(RE::Actor* act);


    inline RE::BGSEquipSlot* right_hand_slot;
    inline RE::BGSEquipSlot* left_hand_slot;

    inline RE::TESBoundObject* unarmed_weapon;

    inline RE::BGSKeyword* switch_keyword_left;
    inline RE::BGSKeyword* switch_keyword_right;
    inline RE::BGSKeyword* unequip_keyword_left;
    inline RE::BGSKeyword* unequip_keyword_right;
    inline RE::BGSKeyword* equip_keyword_left;
    inline RE::BGSKeyword* equip_keyword_right;

    inline RE::SpellItem* unequip_left;
    inline RE::SpellItem* unequip_right;
    inline RE::SpellItem* equip_left;
    inline RE::SpellItem* equip_right;

    inline std::shared_mutex actor_equip_event_mutex;
    inline std::unordered_map<RE::Actor*, EquipEvent> actor_equip_event;

    inline bool player_equip_left = false;

    inline RE::TESObjectREFR* remove_act;
    inline RE::TESBoundObject* remove_obj;
    inline std::chrono::steady_clock::time_point remove_time;
    
    inline RE::Actor* justEquiped_act;
    inline RE::TESBoundObject* justEquiped_obj;
    inline std::chrono::steady_clock::time_point justEquiped_time;

}


#include "Animation.h"
#include "Hooks.h"
#include "MCP.h"
#include "Utils.h"

RE::BSEventNotifyControl AnimationEventSink::ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*) {
    
    const RE::Actor* actor = a_event->holder->As<RE::Actor>();

    if ((!Settings::PC_Switch && actor->IsPlayerRef()) || (!Settings::NPC_Switch && !actor->IsPlayerRef())) {
        Utils::RemoveEvent(actor);
        return RE::BSEventNotifyControl::kContinue;
    }

    if (a_event->tag == "WeaponSheathe" || a_event->tag == "IdleStop") {
        logger::debug("[AnimationEventSink]:[{} - {:08X}] {} Event", actor->GetName(), actor->GetFormID(),
                      std::string(a_event->tag));

        Utils::ExecuteEvent(actor);
    }

    //Safe guard if we wait for WeaponSheathe but instead we get weaponDraw
    if (a_event->tag == "WeapEquip_Out" || a_event->tag == "WeapEquip_OutMoving" || a_event->tag == "tailCombatIdle") {
        logger::debug("[AnimationEventSink]:[{} - {:08X}] {} Event", actor->GetName(), actor->GetFormID(),
                      std::string(a_event->tag));

        RE::Actor* actor_ptr = Utils::GetActor(actor);
        actor_ptr->DrawWeaponMagicHands(false);
        actor_ptr->AsActorState()->actorState2.weaponState = RE::WEAPON_STATE::kWantToSheathe;
    }
    return RE::BSEventNotifyControl::kContinue;
}


RE::BSEventNotifyControl EquipAnimationEventSink::ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*) {
    const RE::Actor* actor = a_event->holder->As<RE::Actor>();

    if (a_event->tag == "WeaponSheathe" || a_event->tag == "IdleStop") {
        Utils::ProceedAnimationInfo(Utils::GetActor(actor));
    }

    if (a_event->tag == "WeapEquip_Out" || a_event->tag == "WeapEquip_OutMoving") {
        logger::debug("[EquipAnimationEventSink]:[{} - {:08X}] {} Event", actor->GetName(), actor->GetFormID(),
                      std::string(a_event->tag));

        actor->RemoveAnimationGraphEventSink(this);
        Utils::RemoveAnimationInfo(Utils::GetActor(actor));
    }
    return RE::BSEventNotifyControl::kContinue;
}

AnimationEventSink* GetOrCreateEventSink(RE::Actor* a_actor) {
    if (!Utils::IsAlreadyTracked(a_actor)) {
        logger::debug("[CreateEventSink]:[{} - {:08X}]", a_actor->GetName(), a_actor->GetFormID());
        const auto eventSink = new AnimationEventSink();
        a_actor->AddAnimationGraphEventSink(eventSink);
        return eventSink;
    } else {
        logger::debug("[GetEventSink]:[{} - {:08X}]", a_actor->GetName(), a_actor->GetFormID());
        return Utils::GetAnimationEventSink(a_actor);
    }
}

void RemoveEventSink(const RE::Actor* a_actor, AnimationEventSink* a_eventSink) {
    logger::debug("[RemoveEventSink]:[{} - {:08X}]", a_actor->GetName(), a_actor->GetFormID());
    a_actor->RemoveAnimationGraphEventSink(a_eventSink);
}



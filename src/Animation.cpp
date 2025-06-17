#include "Animation.h"
#include "Hooks.h"
#include "MCP.h"
#include "Utils.h"

RE::BSEventNotifyControl AnimationEventSink::ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*) {
    const RE::Actor* actor = a_event->holder->As<RE::Actor>();

    if ((!Settings::PC_Switch && actor->IsPlayerRef()) || (!Settings::NPC_Switch && !actor->IsPlayerRef()) ||
        (!Settings::Mod_Active)) {
        Utils::RemoveEvent(actor);
        RemoveEventSink(actor, this);
        return RE::BSEventNotifyControl::kContinue;
    }

    if (a_event->tag == "WeaponSheathe") {
        logger::debug("[AnimationEventSink]:[{} - {:08X}] {} Event", actor->GetName(), actor->GetFormID(),
                      std::string(a_event->tag));

        if (Utils::IsAlreadyTracked(Utils::ConstCastActor(actor))) {
            SKSE::GetTaskInterface()->AddTask([actor]() {
                Utils::ProceedAnimationInfo(Utils::ConstCastActor(actor));
                Utils::ExecuteEvent(actor);
            });
        }
    } else if (a_event->tag == "EnableBumper") { // event called after equip animation is done
        logger::debug("[AnimationEventSink]:[{} - {:08X}] {} Event", actor->GetName(), actor->GetFormID(),
                      std::string(a_event->tag));

        RE::Actor* mutableActor = Utils::ConstCastActor(actor);

        if (!Utils::IsAlreadyTracked(mutableActor)) {  // Remove only if ExecuteEvent was called
            Utils::RemoveAnimationInfo(mutableActor);
            RemoveEventSink(actor, this);
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

AnimationEventSink* GetOrCreateEventSink(RE::Actor* a_actor) {
    if (Utils::IsAlreadyTracked(a_actor)) {
        logger::debug("[GetEventSink]:[{} - {:08X}]", a_actor->GetName(), a_actor->GetFormID());
        return Utils::GetAnimationEventSink(a_actor);
    } else {
        logger::debug("[CreateEventSink]:[{} - {:08X}]", a_actor->GetName(), a_actor->GetFormID());
        const auto eventSink = new AnimationEventSink();
        a_actor->AddAnimationGraphEventSink(eventSink);
        return eventSink;
    }
}

void RemoveEventSink(const RE::Actor* a_actor, AnimationEventSink* a_eventSink) {
    logger::debug("[RemoveEventSink]:[{} - {:08X}]", a_actor->GetName(), a_actor->GetFormID());
    a_actor->RemoveAnimationGraphEventSink(a_eventSink);
}
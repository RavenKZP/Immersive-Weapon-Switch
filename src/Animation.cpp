#include "Animation.h"
#include "Hooks.h"
#include "MCP.h"
#include "Utils.h"

RE::BSEventNotifyControl AnimationEventSink::ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*) {
    if (a_event->tag == "WeaponSheathe") {
        const RE::Actor* actor = a_event->holder->As<RE::Actor>();

        logger::trace("WeaponSheathe Event: {}", actor->GetName());

        Utils::ExecuteEvent(actor);
        RemoveEventSink(actor, this);
    }
    return RE::BSEventNotifyControl::kContinue;
}

void CreateEventSink(RE::Actor* a_actor) {
    const auto eventSink = new AnimationEventSink();
    a_actor->AddAnimationGraphEventSink(eventSink);
}

void RemoveEventSink(const RE::Actor* a_actor, AnimationEventSink* a_eventSink) {
    a_actor->RemoveAnimationGraphEventSink(a_eventSink);
}



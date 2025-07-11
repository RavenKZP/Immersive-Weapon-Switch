#pragma once

struct AnimationEventSink final : RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;
    AnimationEventSink() = default;
};

AnimationEventSink* GetOrCreateEventSink(RE::Actor* actor);
void RemoveEventSink(const RE::Actor* actor, AnimationEventSink* eventSink);

void FakeDrawWeaponMagicHands(RE::Actor* actor, bool equip);
bool SendAnimationEvent(RE::Actor* a_actor, const char* AnimationString);

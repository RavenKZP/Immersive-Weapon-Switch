#pragma once

struct AnimationEventSink final : RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;
    AnimationEventSink() = default;
};

AnimationEventSink* GetOrCreateEventSink(RE::Actor* actor);
void RemoveEventSink(const RE::Actor* actor, AnimationEventSink* eventSink);

struct EquipAnimationEventSink final : RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;
    EquipAnimationEventSink() = default;
};
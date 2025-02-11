#pragma once

namespace Events {

    struct AnimationEventSink final : RE::BSTEventSink<SKSE::ActionEvent> {
        AnimationEventSink(RE::Actor* actor);
        RE::BSEventNotifyControl ProcessEvent(const SKSE::ActionEvent* a_event,
                                              RE::BSTEventSource<SKSE::ActionEvent>*) override;

        RE::Actor* a_actor;
    };

    void CreateEventSink(RE::Actor* a_actor);
    void RemoveEventSink(AnimationEventSink* sink);

    inline std::map<RE::FormID, std::unique_ptr<AnimationEventSink>> actorEventSinks;
}

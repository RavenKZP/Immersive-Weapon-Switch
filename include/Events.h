#ifndef EVENTS_H
#define EVENTS_H


class EquipEventSink : public RE::BSTEventSink<RE::TESEquipEvent> {
    EquipEventSink() = default;
    EquipEventSink(const EquipEventSink&) = delete;
    EquipEventSink(EquipEventSink&&) = delete;
    EquipEventSink& operator=(const EquipEventSink&) = delete;
    EquipEventSink& operator=(EquipEventSink&&) = delete;

public:
    static EquipEventSink* GetSingleton() {
        static EquipEventSink singleton;
        return &singleton;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*);
};

#endif  // EVENTS_H
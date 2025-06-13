#include "EventBus.h"

namespace BGE {

EventBus& EventBus::Instance() {
    static EventBus instance;
    return instance;
}

void EventBus::Clear() {
    m_handlers.clear();
}

} // namespace BGE
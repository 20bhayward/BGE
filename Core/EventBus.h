#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>

namespace BGE {

class EventBus {
public:
    static EventBus& Instance();
    
    template<typename EventType>
    using EventHandler = std::function<void(const EventType&)>;
    
    template<typename EventType>
    void Subscribe(EventHandler<EventType> handler) {
        auto typeId = std::type_index(typeid(EventType));
        auto& handlers = m_handlers[typeId];
        handlers.push_back(std::make_shared<HandlerWrapper<EventType>>(handler));
    }
    
    template<typename EventType>
    void Publish(const EventType& event) {
        auto typeId = std::type_index(typeid(EventType));
        auto it = m_handlers.find(typeId);
        if (it != m_handlers.end()) {
            for (auto& handler : it->second) {
                handler->Call(&event);
            }
        }
    }
    
    void Clear();
    
private:
    EventBus() = default;
    ~EventBus() = default;
    
    class HandlerWrapperBase {
    public:
        virtual ~HandlerWrapperBase() = default;
        virtual void Call(const void* event) = 0;
    };
    
    template<typename EventType>
    class HandlerWrapper : public HandlerWrapperBase {
    public:
        HandlerWrapper(EventHandler<EventType> handler) : m_handler(handler) {}
        
        void Call(const void* event) override {
            m_handler(*static_cast<const EventType*>(event));
        }
        
    private:
        EventHandler<EventType> m_handler;
    };
    
    std::unordered_map<std::type_index, std::vector<std::shared_ptr<HandlerWrapperBase>>> m_handlers;
};

} // namespace BGE
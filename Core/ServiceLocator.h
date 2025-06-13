#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <string>
#include <stdexcept>

namespace BGE {

class ServiceLocator {
public:
    static ServiceLocator& Instance();
    
    template<typename T>
    void RegisterService(std::shared_ptr<T> service) {
        auto typeId = std::type_index(typeid(T));
        m_services[typeId] = service;
    }
    
    template<typename T>
    std::shared_ptr<T> GetService() {
        auto typeId = std::type_index(typeid(T));
        auto it = m_services.find(typeId);
        if (it != m_services.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }
    
    template<typename T>
    bool HasService() {
        auto typeId = std::type_index(typeid(T));
        return m_services.find(typeId) != m_services.end();
    }
    
    template<typename T>
    void UnregisterService() {
        auto typeId = std::type_index(typeid(T));
        m_services.erase(typeId);
    }
    
    void Clear();
    
private:
    ServiceLocator() = default;
    ~ServiceLocator() = default;
    
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_services;
};

} // namespace BGE
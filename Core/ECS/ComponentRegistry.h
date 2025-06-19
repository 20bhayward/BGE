#pragma once

#include "ECSConstants.h"
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <cstdint>
#include <iostream>

namespace BGE {

// Component metadata
struct ComponentInfo {
    ComponentTypeID id = INVALID_COMPONENT_TYPE;
    std::string name;
    size_t size = 0;
    size_t alignment = 0;
    std::type_index typeIndex;
    
    // Function pointers for component operations
    std::function<void*(void*)> constructor;
    std::function<void(void*)> destructor;
    std::function<void(void*, const void*)> copyConstructor;
    std::function<void(void*, void*)> moveConstructor;
    
    ComponentInfo() : typeIndex(typeid(void)) {}
    ComponentInfo(std::type_index idx) : typeIndex(idx) {}
};

class ComponentRegistry {
public:
    static ComponentRegistry& Instance() {
        static ComponentRegistry instance;
        return instance;
    }
    
    // Register a component type
    template<typename T>
    ComponentTypeID RegisterComponent(const std::string& name) {
        auto typeIndex = std::type_index(typeid(T));
        
        // Check if already registered
        auto it = m_typeToID.find(typeIndex);
        if (it != m_typeToID.end()) {
            return it->second;
        }
        
        ComponentTypeID id = m_nextComponentID++;
        
        ComponentInfo info;
        info.id = id;
        info.name = name;
        info.size = sizeof(T);
        info.alignment = alignof(T);
        info.typeIndex = typeIndex;
        
        // Component operations
        info.constructor = [](void* ptr) -> void* {
            return new(ptr) T();
        };
        
        info.destructor = [](void* ptr) {
            static_cast<T*>(ptr)->~T();
        };
        
        info.copyConstructor = [](void* dst, const void* src) {
            new(dst) T(*static_cast<const T*>(src));
        };
        
        info.moveConstructor = [](void* dst, void* src) {
            new(dst) T(std::move(*static_cast<T*>(src)));
        };
        
        m_componentInfos[id] = info;
        m_typeToID[typeIndex] = id;
        m_nameToID[name] = id;
        
        return id;
    }
    
    // Get component type ID
    template<typename T>
    ComponentTypeID GetComponentTypeID() const {
        auto typeIndex = std::type_index(typeid(T));
        auto it = m_typeToID.find(typeIndex);
        if (it != m_typeToID.end()) {
            return it->second;
        }
        return INVALID_COMPONENT_TYPE;
    }
    
    // Get component type ID by name
    ComponentTypeID GetComponentTypeID(const std::string& name) const {
        auto it = m_nameToID.find(name);
        return it != m_nameToID.end() ? it->second : INVALID_COMPONENT_TYPE;
    }
    
    // Get component info
    const ComponentInfo* GetComponentInfo(ComponentTypeID id) const {
        auto it = m_componentInfos.find(id);
        return it != m_componentInfos.end() ? &it->second : nullptr;
    }
    
    // Get all registered components
    const std::unordered_map<ComponentTypeID, ComponentInfo>& GetAllComponents() const {
        return m_componentInfos;
    }
    
    // Get total number of registered components
    size_t GetComponentCount() const { return m_componentInfos.size(); }
    
private:
    ComponentRegistry() = default;
    
    ComponentTypeID m_nextComponentID = 0;
    std::unordered_map<ComponentTypeID, ComponentInfo> m_componentInfos;
    std::unordered_map<std::type_index, ComponentTypeID> m_typeToID;
    std::unordered_map<std::string, ComponentTypeID> m_nameToID;
};

// Helper macro for component registration
#define REGISTER_COMPONENT(Type, Name) \
    BGE::ComponentRegistry::Instance().RegisterComponent<Type>(Name)

} // namespace BGE
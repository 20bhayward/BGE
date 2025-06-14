#include "Components.h"
#include "Logger.h"
#include <sstream>

namespace BGE {

// Helper functions for simple serialization
namespace {
    std::string VectorToString(const Vector3& vec) {
        std::stringstream ss;
        ss << vec.x << "," << vec.y << "," << vec.z;
        return ss.str();
    }
    
    Vector3 StringToVector(const std::string& str) {
        std::stringstream ss(str);
        std::string item;
        Vector3 vec{0, 0, 0};
        
        if (std::getline(ss, item, ',')) vec.x = std::stof(item);
        if (std::getline(ss, item, ',')) vec.y = std::stof(item);
        if (std::getline(ss, item, ',')) vec.z = std::stof(item);
        
        return vec;
    }
    
    std::string ChildrenToString(const std::vector<EntityID>& children) {
        std::stringstream ss;
        for (size_t i = 0; i < children.size(); ++i) {
            if (i > 0) ss << ",";
            ss << children[i];
        }
        return ss.str();
    }
    
    std::vector<EntityID> StringToChildren(const std::string& str) {
        std::vector<EntityID> children;
        if (str.empty()) return children;
        
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            children.push_back(std::stoull(item));
        }
        return children;
    }
}

// TransformComponent implementation
SerializationData TransformComponent::Serialize() const {
    SerializationData data;
    
    data["position"] = VectorToString(position);
    data["rotation"] = std::to_string(rotation);
    data["scale"] = VectorToString(scale);
    data["parent"] = std::to_string(parent);
    data["children"] = ChildrenToString(children);
    
    return data;
}

void TransformComponent::Deserialize(const SerializationData& data) {
    auto it = data.find("position");
    if (it != data.end()) {
        position = StringToVector(it->second);
    }
    
    it = data.find("rotation");
    if (it != data.end()) {
        rotation = std::stof(it->second);
    }
    
    it = data.find("scale");
    if (it != data.end()) {
        scale = StringToVector(it->second);
    }
    
    it = data.find("parent");
    if (it != data.end()) {
        parent = std::stoull(it->second);
    }
    
    it = data.find("children");
    if (it != data.end()) {
        children = StringToChildren(it->second);
    }
}

void TransformComponent::AddChild(EntityID child) {
    if (std::find(children.begin(), children.end(), child) == children.end()) {
        children.push_back(child);
    }
}

void TransformComponent::RemoveChild(EntityID child) {
    children.erase(std::remove(children.begin(), children.end(), child), children.end());
}

void TransformComponent::SetParent(EntityID newParent) {
    if (parent != INVALID_ENTITY_ID) {
        // Remove from old parent's children list
        auto& entityManager = EntityManager::Instance();
        Entity* oldParentEntity = entityManager.GetEntity(parent);
        if (oldParentEntity) {
            TransformComponent* oldParentTransform = oldParentEntity->GetComponent<TransformComponent>();
            if (oldParentTransform) {
                oldParentTransform->RemoveChild(GetEntityID());
            }
        }
    }
    
    parent = newParent;
    
    if (parent != INVALID_ENTITY_ID) {
        // Add to new parent's children list
        auto& entityManager = EntityManager::Instance();
        Entity* newParentEntity = entityManager.GetEntity(parent);
        if (newParentEntity) {
            TransformComponent* newParentTransform = newParentEntity->GetComponent<TransformComponent>();
            if (newParentTransform) {
                newParentTransform->AddChild(GetEntityID());
            }
        }
    }
}

} // namespace BGE
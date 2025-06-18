#include "Components.h"
#include "Logger.h"
#include "Entity.h"  // Need full definition before EntityManager.h
#include "ECS/EntityManager.h"
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
    
    std::string QuaternionToString(const Quaternion& q) {
        std::stringstream ss;
        ss << q.x << "," << q.y << "," << q.z << "," << q.w;
        return ss.str();
    }
    
    Quaternion StringToQuaternion(const std::string& str) {
        std::stringstream ss(str);
        std::string item;
        Quaternion q;
        
        if (std::getline(ss, item, ',')) q.x = std::stof(item);
        if (std::getline(ss, item, ',')) q.y = std::stof(item);
        if (std::getline(ss, item, ',')) q.z = std::stof(item);
        if (std::getline(ss, item, ',')) q.w = std::stof(item);
        
        return q;
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
            children.push_back(EntityID(std::stoull(item)));
        }
        return children;
    }
}

// TransformComponent implementation
SerializationData TransformComponent::Serialize() const {
    SerializationData data;
    
    data["position"] = VectorToString(position);
    data["rotation"] = std::to_string(rotation); // Keep for backward compatibility
    data["rotation3D"] = QuaternionToString(rotation3D);
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
    
    // Try to load 3D rotation first
    it = data.find("rotation3D");
    if (it != data.end()) {
        rotation3D = StringToQuaternion(it->second);
        rotation = rotation3D.ToEuler().z; // Update 2D rotation
    } else {
        // Fall back to 2D rotation for backward compatibility
        it = data.find("rotation");
        if (it != data.end()) {
            rotation = std::stof(it->second);
            rotation3D = Quaternion::FromEuler(0.0f, 0.0f, rotation);
        }
    }
    
    it = data.find("scale");
    if (it != data.end()) {
        scale = StringToVector(it->second);
    }
    
    it = data.find("parent");
    if (it != data.end()) {
        parent = EntityID(std::stoull(it->second));
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
    // TODO: Implement hierarchy management with new ECS system
    // For now, just set the parent without updating relationships
    parent = newParent;
}

} // namespace BGE
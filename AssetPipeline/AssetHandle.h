#pragma once

#include <string>
#include <functional>

namespace BGE {

// UUID-based asset handle for persistent asset references
class AssetHandle {
public:
    AssetHandle() = default;
    explicit AssetHandle(const std::string& uuid) : m_uuid(uuid) {}
    
    static AssetHandle Generate();
    static AssetHandle FromString(const std::string& uuidStr);
    
    bool IsValid() const { return !m_uuid.empty(); }
    const std::string& ToString() const { return m_uuid; }
    
    bool operator==(const AssetHandle& other) const { return m_uuid == other.m_uuid; }
    bool operator!=(const AssetHandle& other) const { return !(*this == other); }
    bool operator<(const AssetHandle& other) const { return m_uuid < other.m_uuid; }

private:
    std::string m_uuid;
};

// Hash function for using AssetHandle in unordered containers
struct AssetHandleHash {
    std::size_t operator()(const AssetHandle& handle) const {
        return std::hash<std::string>{}(handle.ToString());
    }
};

} // namespace BGE
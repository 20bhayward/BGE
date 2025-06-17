#pragma once

#include "../AssetTypes.h"
#include <unordered_map>
#include <string>

namespace BGE {

// Simple icon management system for the UI
class IconManager {
public:
    static IconManager& Instance();
    
    bool Initialize();
    void Shutdown();
    
    // Get icon texture ID for asset type
    uint32_t GetIcon(AssetType type) const;
    
    // Get FontAwesome icon Unicode character for asset type
    const char* GetFontAwesomeIcon(AssetType type) const;
    
    // Get text representation for assets without icons
    std::string GetIconText(AssetType type) const;
    
    // Create simple colored icons programmatically
    uint32_t CreateColorIcon(float r, float g, float b, int size = 16);
    
    // Create material thumbnail from color data
    uint32_t CreateMaterialThumbnail(const float* color, float roughness, float metallic, int size = 64);

private:
    IconManager() = default;
    ~IconManager() = default;
    
    void CreateDefaultIcons();
    uint32_t CreateTextIcon(const std::string& text, float r, float g, float b, int size = 16);
    
    std::unordered_map<AssetType, uint32_t> m_icons;
    std::unordered_map<AssetType, std::string> m_iconTexts;
    std::unordered_map<AssetType, const char*> m_fontAwesomeIcons;
};

} // namespace BGE
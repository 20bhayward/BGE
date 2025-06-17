#include "IconManager.h"
#include "../ServiceLocator.h"
#include "../../Renderer/Renderer.h"
#include <vector>
#include <cstring>

namespace BGE {

IconManager& IconManager::Instance() {
    static IconManager instance;
    return instance;
}

bool IconManager::Initialize() {
    // Initialize FontAwesome Unicode characters for icons
    m_fontAwesomeIcons[AssetType::Unknown] = "\uf15b";        // fa-file
    m_fontAwesomeIcons[AssetType::Folder] = "\uf07b";         // fa-folder
    m_fontAwesomeIcons[AssetType::Texture] = "\uf1c5";        // fa-file-image
    m_fontAwesomeIcons[AssetType::Material] = "\uf53f";       // fa-palette
    m_fontAwesomeIcons[AssetType::Scene] = "\uf0ac";          // fa-globe
    m_fontAwesomeIcons[AssetType::Audio] = "\uf1c7";          // fa-file-audio
    m_fontAwesomeIcons[AssetType::Script] = "\uf1c9";         // fa-file-code
    m_fontAwesomeIcons[AssetType::Prefab] = "\uf1b2";         // fa-cube
    m_fontAwesomeIcons[AssetType::Model] = "\uf1b3";          // fa-cubes
    m_fontAwesomeIcons[AssetType::Animation] = "\uf008";      // fa-film
    
    // Fallback ASCII representations
    m_iconTexts[AssetType::Unknown] = "FILE";
    m_iconTexts[AssetType::Folder] = "DIR";  
    m_iconTexts[AssetType::Texture] = "IMG";
    m_iconTexts[AssetType::Material] = "MAT";
    m_iconTexts[AssetType::Scene] = "SCN";
    m_iconTexts[AssetType::Audio] = "AUD";
    m_iconTexts[AssetType::Script] = "CPP";
    m_iconTexts[AssetType::Prefab] = "PFB";
    m_iconTexts[AssetType::Model] = "MDL";
    m_iconTexts[AssetType::Animation] = "ANM";
    
    CreateDefaultIcons();
    return true;
}

void IconManager::Shutdown() {
    auto renderer = ServiceLocator::Instance().GetService<Renderer>();
    if (renderer) {
        for (const auto& pair : m_icons) {
            if (pair.second != 0) {
                renderer->DeleteTexture(pair.second);
            }
        }
    }
    
    m_icons.clear();
    m_iconTexts.clear();
}

uint32_t IconManager::GetIcon(AssetType type) const {
    auto it = m_icons.find(type);
    return it != m_icons.end() ? it->second : 0;
}

const char* IconManager::GetFontAwesomeIcon(AssetType type) const {
    auto it = m_fontAwesomeIcons.find(type);
    return it != m_fontAwesomeIcons.end() ? it->second : "\uf15b"; // Default to file icon
}

std::string IconManager::GetIconText(AssetType type) const {
    auto it = m_iconTexts.find(type);
    return it != m_iconTexts.end() ? it->second : "FILE";
}

uint32_t IconManager::CreateColorIcon(float r, float g, float b, int size) {
    std::vector<unsigned char> pixels(size * size * 4);
    
    unsigned char red = static_cast<unsigned char>(r * 255);
    unsigned char green = static_cast<unsigned char>(g * 255);
    unsigned char blue = static_cast<unsigned char>(b * 255);
    unsigned char alpha = 255;
    
    for (int i = 0; i < size * size; ++i) {
        pixels[i * 4 + 0] = red;
        pixels[i * 4 + 1] = green;
        pixels[i * 4 + 2] = blue;
        pixels[i * 4 + 3] = alpha;
    }
    
    auto renderer = ServiceLocator::Instance().GetService<Renderer>();
    if (renderer) {
        return renderer->CreateTexture(size, size, 4, pixels.data());
    }
    
    return 0;
}

void IconManager::CreateDefaultIcons() {
    // Create simple colored icons for each asset type
    m_icons[AssetType::Unknown] = CreateColorIcon(0.5f, 0.5f, 0.5f);      // Gray
    m_icons[AssetType::Folder] = CreateColorIcon(1.0f, 0.8f, 0.3f);       // Yellow
    m_icons[AssetType::Texture] = CreateColorIcon(0.3f, 0.7f, 1.0f);      // Blue
    m_icons[AssetType::Material] = CreateColorIcon(0.8f, 0.3f, 0.8f);     // Purple
    m_icons[AssetType::Scene] = CreateColorIcon(0.3f, 0.8f, 0.3f);        // Green
    m_icons[AssetType::Audio] = CreateColorIcon(1.0f, 0.5f, 0.2f);        // Orange
    m_icons[AssetType::Script] = CreateColorIcon(0.9f, 0.9f, 0.9f);       // White
    m_icons[AssetType::Prefab] = CreateColorIcon(0.6f, 0.4f, 0.8f);       // Light Purple
    m_icons[AssetType::Model] = CreateColorIcon(0.7f, 0.6f, 0.4f);        // Brown
    m_icons[AssetType::Animation] = CreateColorIcon(1.0f, 0.3f, 0.3f);    // Red
}

uint32_t IconManager::CreateMaterialThumbnail(const float* color, float roughness, float metallic, int size) {
    std::vector<unsigned char> pixels(size * size * 4);
    
    // Base material color
    unsigned char red = static_cast<unsigned char>(color[0] * 255);
    unsigned char green = static_cast<unsigned char>(color[1] * 255);
    unsigned char blue = static_cast<unsigned char>(color[2] * 255);
    unsigned char alpha = static_cast<unsigned char>(color[3] * 255);
    
    // Create a simple gradient/pattern to show material properties
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int idx = (y * size + x) * 4;
            
            // Base color
            pixels[idx + 0] = red;
            pixels[idx + 1] = green;
            pixels[idx + 2] = blue;
            pixels[idx + 3] = alpha;
            
            // Add roughness pattern (noise in top half)
            if (y < size / 2) {
                float roughnessFactor = roughness * 0.3f;
                int noise = (x + y) % 3 == 0 ? 1 : 0;
                pixels[idx + 0] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, red * (1.0f - roughnessFactor * noise))));
                pixels[idx + 1] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, green * (1.0f - roughnessFactor * noise))));
                pixels[idx + 2] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, blue * (1.0f - roughnessFactor * noise))));
            }
            
            // Add metallic reflection (bright spot in bottom-right)
            if (metallic > 0.1f && x > size * 0.6f && y > size * 0.6f) {
                float metallicFactor = metallic * 0.8f;
                pixels[idx + 0] = static_cast<unsigned char>(std::min(255.0f, red + 255 * metallicFactor));
                pixels[idx + 1] = static_cast<unsigned char>(std::min(255.0f, green + 255 * metallicFactor));
                pixels[idx + 2] = static_cast<unsigned char>(std::min(255.0f, blue + 255 * metallicFactor));
            }
        }
    }
    
    auto renderer = ServiceLocator::Instance().GetService<Renderer>();
    if (renderer) {
        return renderer->CreateTexture(size, size, 4, pixels.data());
    }
    
    return 0;
}

uint32_t IconManager::CreateTextIcon(const std::string& text, float r, float g, float b, int size) {
    // For now, just create a colored square
    // In a full implementation, this would render text to a texture
    (void)text; // Suppress unused parameter warning
    return CreateColorIcon(r, g, b, size);
}

} // namespace BGE
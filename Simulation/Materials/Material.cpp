#include "Material.h"

namespace BGE {

Material::Material(MaterialID id, const std::string& name) 
    : m_id(id), m_name(name) {
}

void Material::SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // Store as RGBA (Red in lowest bits)
    m_color = (static_cast<uint32_t>(a) << 24) | 
              (static_cast<uint32_t>(b) << 16) | 
              (static_cast<uint32_t>(g) << 8) | 
              static_cast<uint32_t>(r);
}

void Material::AddReaction(const MaterialReaction& reaction) {
    m_reactions.push_back(reaction);
}

} // namespace BGE
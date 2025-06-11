#pragma once

#include <string>
#include <vector>
#include <SFML/Graphics.hpp> // Added for sf::Color
#include "Grid.h"            // Added for MaterialID

// Material struct now includes color
struct Material {
    std::string name;
    sf::Color color; // Added color member
    // Add other material properties here (e.g., density, etc.)
=======
#include <SFML/Graphics/Color.hpp> // Added for sf::Color

enum class MaterialID { Empty, Sand, Rock, Water };

struct MaterialDefinition {
    MaterialID id;
    std::string name;
    sf::Color color;
    float density;
};

// Color Constants
const sf::Color COLOR_EMPTY = sf::Color::Black;
const sf::Color COLOR_SAND = sf::Color::Yellow;
const sf::Color COLOR_ROCK = sf::Color(128, 128, 128); // Gray
const sf::Color COLOR_WATER = sf::Color::Blue;

class MaterialRegistry {
public:
    MaterialRegistry();
    ~MaterialRegistry();

    // registerMaterial might need to change to accept MaterialID and sf::Color
    void registerMaterial(MaterialID id, const std::string& name, sf::Color color);
    // getMaterial might change or be supplemented by a method using MaterialID
    const Material* getMaterial(const std::string& name) const; // Kept for now
    sf::Color getColor(MaterialID id) const; // Added method

    // Add other material management methods here

private:
    // Assuming MaterialID can be used as an index for simplicity for now.
    // This implies MaterialID values are 0, 1, 2...
    // A std::map<MaterialID, Material> might be more robust.
    std::vector<Material> m_materials;
    // To make getColor efficient, we might need a direct mapping from MaterialID to color.
    // For now, we can store Materials in the vector such that their index matches the MaterialID.
    // This requires careful registration.
=======
    const MaterialDefinition& getMaterial(MaterialID id) const;
    // Add other material management methods here
  
    std::vector<MaterialDefinition> m_materials; // Changed Material to MaterialDefinition

};

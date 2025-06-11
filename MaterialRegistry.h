#pragma once

#include <string>
#include <vector>
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

    const MaterialDefinition& getMaterial(MaterialID id) const;
    // Add other material management methods here

private:
    std::vector<MaterialDefinition> m_materials; // Changed Material to MaterialDefinition
};

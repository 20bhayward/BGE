#include "MaterialDatabase.h"
#include "MaterialSystem.h"
#include "../../ThirdParty/json/json.hpp" // Changed: Added json.hpp include
#include <fstream>
#include <iostream> // Changed: Kept iostream for error reporting
#include "../../Core/Logger.h" // Add BGE logging
#ifdef _WIN32
#include <direct.h> // For _getcwd on Windows
#define getcwd _getcwd
#else
#include <unistd.h> // For getcwd on Unix
#endif

namespace BGE {

MaterialDatabase::MaterialDatabase() = default;
MaterialDatabase::~MaterialDatabase() = default;

bool MaterialDatabase::LoadFromFile(const std::string& filepath, MaterialSystem& materialSystem) {
    // Debug: Log current working directory and file path
    char* cwd = getcwd(nullptr, 0);
    BGE_LOG_INFO("MaterialDatabase", "Current working directory: " + std::string(cwd ? cwd : "unknown"));
    BGE_LOG_INFO("MaterialDatabase", "Attempting to load file: " + filepath);
    if (cwd) free(cwd);
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        BGE_LOG_ERROR("MaterialDatabase", "Failed to open material database file: " + filepath);
        return false;
    }

    nlohmann::json jsonData;
    try {
        BGE_LOG_INFO("MaterialDatabase", "File opened successfully, attempting JSON parse...");
        file >> jsonData;
        BGE_LOG_INFO("MaterialDatabase", "JSON parsed successfully, found " + std::to_string(jsonData.size()) + " top-level items");
    } catch (const nlohmann::json::parse_error& e) {
        BGE_LOG_ERROR("MaterialDatabase", "JSON parse error: " + std::string(e.what()));
        std::cerr << "Error: Failed to parse JSON from file: " << filepath << "\n"
                  << "Details: " << e.what() << std::endl;
        return false;
    }

    if (!jsonData.contains("materials") || !jsonData["materials"].is_array()) {
        std::cerr << "Error: JSON file does not contain a 'materials' array: " << filepath << std::endl;
        return false;
    }

    BGE_LOG_INFO("MaterialDatabase", "Found materials array with " + std::to_string(jsonData["materials"].size()) + " entries");

    for (const auto& materialEntry : jsonData["materials"]) {
        try {
            BGE_LOG_INFO("MaterialDatabase", "Processing material entry...");
            std::string name = materialEntry.at("name").get<std::string>();
            BGE_LOG_INFO("MaterialDatabase", "Material name: " + name);
            std::string behaviorStr = materialEntry.at("behavior").get<std::string>();
            float density = materialEntry.at("density").get<float>();
            auto colorArray = materialEntry.at("color").get<std::vector<uint8_t>>();
            int hotkey = materialEntry.value("hotkey", 0); // Default to 0 if not present

            if (colorArray.size() != 4) {
                std::cerr << "Error: Color array for material '" << name << "' must have 4 components (RGBA)." << std::endl;
                continue;
            }

            MaterialBehavior behavior;
            if (behaviorStr == "Powder") {
                behavior = MaterialBehavior::Powder;
            } else if (behaviorStr == "Liquid") {
                behavior = MaterialBehavior::Liquid;
            } else if (behaviorStr == "Static") {
                behavior = MaterialBehavior::Static;
            } else if (behaviorStr == "Gas") {
                behavior = MaterialBehavior::Gas;
            } else if (behaviorStr == "Fire") {
                behavior = MaterialBehavior::Fire;
            } else {
                std::cerr << "Warning: Unknown material behavior '" << behaviorStr << "' for material '" << name << "'. Defaulting to Static." << std::endl;
                behavior = MaterialBehavior::Static;
            }

            auto builder = materialSystem.CreateMaterialBuilder(name)
                .SetBehavior(behavior)
                .SetDensity(density)
                .SetColor(colorArray[0], colorArray[1], colorArray[2], colorArray[3]); // Corrected SetColor call

            if (hotkey != 0) {
                builder.SetHotKey(hotkey); // Use the new SetHotKey method
            }

            // builder.Build(); // Build() is not part of MaterialBuilder in MaterialSystem.h; settings are applied directly.

            // Get the material to add reactions
            MaterialID currentMaterialID = materialSystem.GetMaterialID(name);
            if (!materialSystem.HasMaterial(currentMaterialID)) {
                std::cerr << "Error: Material '" << name << "' was not properly created or registered." << std::endl;
                continue;
            }
            Material& currentMaterial = materialSystem.GetMaterial(currentMaterialID);

            // Check for reactions
            if (materialEntry.contains("reactions") && materialEntry["reactions"].is_array()) {
                for (const auto& reactionEntry : materialEntry["reactions"]) {
                    try {
                        std::string reactantName = reactionEntry.at("reactant").get<std::string>();
                        std::string product1Name = reactionEntry.at("product1").get<std::string>();
                        std::string product2Name = reactionEntry.value("product2", ""); // Default to empty if missing or null

                        double probability = reactionEntry.at("probability").get<double>();
                        bool requiresHeat = reactionEntry.at("requiresHeat").get<bool>();
                        float minTemperature = reactionEntry.at("minTemperature").get<float>();

                        MaterialID reactantID = materialSystem.GetMaterialID(reactantName);
                        MaterialID product1ID = materialSystem.GetMaterialID(product1Name);
                        MaterialID product2ID = BGE::MATERIAL_EMPTY;

                        if (reactantID == BGE::MATERIAL_EMPTY && reactantName != "Empty") { // Allow "Empty" to be a valid reactant if needed, though typically not. Or handle if GetMaterialID returns MATERIAL_EMPTY for unknown.
                            std::cerr << "Error: Unknown reactant material '" << reactantName << "' (ID: " << reactantID << ") in reaction for '" << name << "'." << std::endl;
                            continue;
                        }
                        if (product1ID == BGE::MATERIAL_EMPTY && product1Name != "Empty") {
                            std::cerr << "Error: Unknown product1 material '" << product1Name << "' (ID: " << product1ID << ") in reaction for '" << name << "'." << std::endl;
                            continue;
                        }

                        if (!product2Name.empty()) {
                            product2ID = materialSystem.GetMaterialID(product2Name);
                            if (product2ID == BGE::MATERIAL_EMPTY && product2Name != "Empty") {
                                std::cerr << "Warning: Unknown product2 material '" << product2Name << "' (ID: " << product2ID << ") in reaction for '" << name << "'. Setting to MATERIAL_EMPTY." << std::endl;
                                // product2ID is already MATERIAL_EMPTY, so just a warning.
                            }
                        }

                        BGE::MaterialReaction reaction;
                        reaction.reactant = reactantID;
                        reaction.product1 = product1ID;
                        reaction.product2 = product2ID;
                        reaction.probability = static_cast<float>(probability); // Assuming probability in struct is float
                        reaction.requiresHeat = requiresHeat;
                        reaction.minTemperature = minTemperature;

                        currentMaterial.AddReaction(reaction);

                    } catch (const nlohmann::json::exception& re) {
                        std::cerr << "Error parsing reaction for material '" << name << "': " << re.what() << std::endl;
                    }
                }
            }

        } catch (const nlohmann::json::exception& e) {
            std::string materialNameHint = "unknown (error before reading name)";
            if (materialEntry.contains("name") && materialEntry["name"].is_string()) {
                materialNameHint = materialEntry["name"].get<std::string>();
            }
            std::cerr << "Error: Failed to parse material entry for '" << materialNameHint << "': " << e.what() << std::endl;
            // Decide if one bad entry should stop all loading or just skip this one.
            // For now, we skip.
        }
    }
    // LoadBasicMaterials(materialSystem); // We are now loading from JSON, so this might not be needed or could be supplemental
    BGE_LOG_INFO("MaterialDatabase", "LoadFromFile completed successfully!");
    return true;
}

bool MaterialDatabase::SaveToFile(const std::string& filepath, const MaterialSystem& materialSystem) {
    (void)materialSystem;
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to create material database file: " << filepath << std::endl;
        return false;
    }
    
    // TODO: Implement JSON serialization
    file << "{ \"materials\": [] }\n";
    return true;
}

void MaterialDatabase::LoadBasicMaterials(MaterialSystem& materialSystem) {
    // Sand
    auto sand = materialSystem.CreateMaterialBuilder("Sand")
        .SetColor(194, 178, 128)
        .SetBehavior(MaterialBehavior::Powder)
        .SetDensity(1.5f);
    
    // Water
    auto water = materialSystem.CreateMaterialBuilder("Water")
        .SetColor(64, 164, 223, 180)
        .SetBehavior(MaterialBehavior::Liquid)
        .SetDensity(1.0f);
    
    // Fire
    auto fire = materialSystem.CreateMaterialBuilder("Fire")
        .SetColor(255, 100, 0)
        .SetBehavior(MaterialBehavior::Fire)
        .SetDensity(0.1f)
        .SetEmission(2.0f);
    
    // Wood
    auto wood = materialSystem.CreateMaterialBuilder("Wood")
        .SetColor(139, 69, 19)
        .SetBehavior(MaterialBehavior::Static)
        .SetDensity(0.8f);
    
    // Stone
    auto stone = materialSystem.CreateMaterialBuilder("Stone")
        .SetColor(128, 128, 128)
        .SetBehavior(MaterialBehavior::Static)
        .SetDensity(2.5f);
}

void MaterialDatabase::LoadAdvancedMaterials(MaterialSystem& materialSystem) {
    // Oil
    auto oil = materialSystem.CreateMaterialBuilder("Oil")
        .SetColor(64, 32, 16)
        .SetBehavior(MaterialBehavior::Liquid)
        .SetDensity(0.8f);
    
    // Steam
    auto steam = materialSystem.CreateMaterialBuilder("Steam")
        .SetColor(200, 200, 255, 100)
        .SetBehavior(MaterialBehavior::Gas)
        .SetDensity(0.001f);
    
    // Metal
    auto metal = materialSystem.CreateMaterialBuilder("Metal")
        .SetColor(192, 192, 192)
        .SetBehavior(MaterialBehavior::Static)
        .SetDensity(7.8f);
}

void MaterialDatabase::LoadChemicalMaterials(MaterialSystem& materialSystem) {
    // Acid
    auto acid = materialSystem.CreateMaterialBuilder("Acid")
        .SetColor(0, 255, 0, 200)
        .SetBehavior(MaterialBehavior::Liquid)
        .SetDensity(1.2f);
    
    // Lava
    auto lava = materialSystem.CreateMaterialBuilder("Lava")
        .SetColor(255, 69, 0)
        .SetBehavior(MaterialBehavior::Liquid)
        .SetDensity(3.0f)
        .SetEmission(3.0f);
}

bool MaterialDatabase::ValidateDatabase(const MaterialSystem& materialSystem) const {
    // Check if basic materials exist
    const std::vector<std::string> requiredMaterials = {
        "Empty", "Sand", "Water", "Fire", "Wood", "Stone"
    };
    
    for (const std::string& name : requiredMaterials) {
        if (!materialSystem.HasMaterial(name)) {
            std::cerr << "Required material missing: " << name << std::endl;
            return false;
        }
    }
    
    return true;
}

bool MaterialDatabase::ParseMaterialData(const std::string& jsonData, std::vector<MaterialData>& materials) {
    (void)jsonData;
    (void)materials;
    // TODO: Implement JSON parsing
    return false;
}

std::string MaterialDatabase::SerializeMaterialData(const std::vector<MaterialData>& materials) {
    (void)materials;
    // TODO: Implement JSON serialization
    return "{}";
}

void MaterialDatabase::CreateMaterialFromData(const MaterialData& data, MaterialSystem& materialSystem) {
    (void)data; (void)materialSystem;
    // TODO: Create material from parsed data
}

} // namespace BGE
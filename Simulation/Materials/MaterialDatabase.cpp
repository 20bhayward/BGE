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

    // Store reaction data for later processing after all materials are loaded
    std::vector<std::pair<std::string, nlohmann::json>> pendingReactions;

    // First pass: Create all materials without reactions
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

            // Parse physical properties if present
            if (materialEntry.contains("physicalProperties") && materialEntry["physicalProperties"].is_object()) {
                auto physicalProps = materialEntry["physicalProperties"];
                MaterialID currentMaterialID = materialSystem.GetMaterialID(name);
                if (materialSystem.HasMaterial(currentMaterialID)) {
                    Material& currentMaterial = materialSystem.GetMaterial(currentMaterialID);
                    
                    if (physicalProps.contains("hardness")) {
                        currentMaterial.SetHardness(physicalProps["hardness"].get<float>());
                    }
                    if (physicalProps.contains("explosiveResistance")) {
                        currentMaterial.SetExplosiveResistance(physicalProps["explosiveResistance"].get<float>());
                    }
                }
            }

            // Parse reactive properties if present
            if (materialEntry.contains("reactiveProperties") && materialEntry["reactiveProperties"].is_object()) {
                auto reactiveProps = materialEntry["reactiveProperties"];
                MaterialID currentMaterialID = materialSystem.GetMaterialID(name);
                if (materialSystem.HasMaterial(currentMaterialID)) {
                    Material& currentMaterial = materialSystem.GetMaterial(currentMaterialID);
                    
                    if (reactiveProps.contains("acidity")) {
                        currentMaterial.SetAcidity(reactiveProps["acidity"].get<float>());
                    }
                    if (reactiveProps.contains("reactivity")) {
                        currentMaterial.SetReactivity(reactiveProps["reactivity"].get<float>());
                    }
                    if (reactiveProps.contains("volatility")) {
                        currentMaterial.SetVolatility(reactiveProps["volatility"].get<float>());
                    }
                }
            }

            // Parse visual pattern if present
            if (materialEntry.contains("visualPattern") && materialEntry["visualPattern"].is_object()) {
                auto visualPattern = materialEntry["visualPattern"];
                
                // Parse pattern type
                if (visualPattern.contains("pattern")) {
                    std::string patternStr = visualPattern["pattern"].get<std::string>();
                    VisualPattern pattern = VisualPattern::Solid;
                    
                    if (patternStr == "Speck") pattern = VisualPattern::Speck;
                    else if (patternStr == "Wavy") pattern = VisualPattern::Wavy;
                    else if (patternStr == "Line") pattern = VisualPattern::Line;
                    else if (patternStr == "Border") pattern = VisualPattern::Border;
                    else if (patternStr == "Gradient") pattern = VisualPattern::Gradient;
                    else if (patternStr == "Checkerboard") pattern = VisualPattern::Checkerboard;
                    else if (patternStr == "Dots") pattern = VisualPattern::Dots;
                    else if (patternStr == "Stripes") pattern = VisualPattern::Stripes;
                    else if (patternStr == "Noise") pattern = VisualPattern::Noise;
                    else if (patternStr == "Marble") pattern = VisualPattern::Marble;
                    else if (patternStr == "Crystal") pattern = VisualPattern::Crystal;
                    else if (patternStr == "Honeycomb") pattern = VisualPattern::Honeycomb;
                    else if (patternStr == "Spiral") pattern = VisualPattern::Spiral;
                    else if (patternStr == "Ripple") pattern = VisualPattern::Ripple;
                    else if (patternStr == "Flame") pattern = VisualPattern::Flame;
                    else if (patternStr == "Wood") pattern = VisualPattern::Wood;
                    else if (patternStr == "Metal") pattern = VisualPattern::Metal;
                    else if (patternStr == "Fabric") pattern = VisualPattern::Fabric;
                    else if (patternStr == "Scale") pattern = VisualPattern::Scale;
                    else if (patternStr == "Bubble") pattern = VisualPattern::Bubble;
                    else if (patternStr == "Crack") pattern = VisualPattern::Crack;
                    else if (patternStr == "Flow") pattern = VisualPattern::Flow;
                    else if (patternStr == "Spark") pattern = VisualPattern::Spark;
                    else if (patternStr == "Glow") pattern = VisualPattern::Glow;
                    else if (patternStr == "Frost") pattern = VisualPattern::Frost;
                    else if (patternStr == "Sand") pattern = VisualPattern::Sand;
                    else if (patternStr == "Rock") pattern = VisualPattern::Rock;
                    else if (patternStr == "Plasma") pattern = VisualPattern::Plasma;
                    else if (patternStr == "Lightning") pattern = VisualPattern::Lightning;
                    else if (patternStr == "Smoke") pattern = VisualPattern::Smoke;
                    else if (patternStr == "Steam") pattern = VisualPattern::Steam;
                    else if (patternStr == "Oil") pattern = VisualPattern::Oil;
                    else if (patternStr == "Blood") pattern = VisualPattern::Blood;
                    else if (patternStr == "Acid") pattern = VisualPattern::Acid;
                    else if (patternStr == "Ice") pattern = VisualPattern::Ice;
                    else if (patternStr == "Lava") pattern = VisualPattern::Lava;
                    else if (patternStr == "Gas") pattern = VisualPattern::Gas;
                    else if (patternStr == "Liquid") pattern = VisualPattern::Liquid;
                    else if (patternStr == "Powder") pattern = VisualPattern::Powder;
                    
                    // Get the material to apply visual pattern
                    MaterialID currentMaterialID = materialSystem.GetMaterialID(name);
                    if (materialSystem.HasMaterial(currentMaterialID)) {
                        Material& currentMaterial = materialSystem.GetMaterial(currentMaterialID);
                        currentMaterial.SetVisualPattern(pattern);
                        
                        // Parse secondary color if present
                        if (visualPattern.contains("secondaryColor")) {
                            auto secColorArray = visualPattern["secondaryColor"].get<std::vector<uint8_t>>();
                            if (secColorArray.size() == 4) {
                                currentMaterial.SetSecondaryColor(secColorArray[0], secColorArray[1], secColorArray[2], secColorArray[3]);
                            }
                        }
                        
                        // Parse other pattern properties if present
                        if (visualPattern.contains("patternScale")) {
                            // Note: Would need to add setter for patternScale if needed
                        }
                        if (visualPattern.contains("patternIntensity")) {
                            // Note: Would need to add setter for patternIntensity if needed
                        }
                        if (visualPattern.contains("animationSpeed")) {
                            // Note: Would need to add setter for animationSpeed if needed
                        }
                    }
                }
            }

            // Get the material to add reactions
            MaterialID currentMaterialID = materialSystem.GetMaterialID(name);
            if (!materialSystem.HasMaterial(currentMaterialID)) {
                std::cerr << "Error: Material '" << name << "' was not properly created or registered." << std::endl;
                continue;
            }
            // Store reactions for second pass processing
            if (materialEntry.contains("reactions") && materialEntry["reactions"].is_array()) {
                pendingReactions.emplace_back(name, materialEntry["reactions"]);
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
    
    // Second pass: Process all reactions now that all materials exist
    BGE_LOG_INFO("MaterialDatabase", "Processing " + std::to_string(pendingReactions.size()) + " materials with reactions...");
    
    for (const auto& reactionData : pendingReactions) {
        const std::string& materialName = reactionData.first;
        const nlohmann::json& reactions = reactionData.second;
        
        MaterialID materialID = materialSystem.GetMaterialID(materialName);
        if (materialID == BGE::MATERIAL_EMPTY) {
            std::cerr << "Error: Could not find material '" << materialName << "' for reaction processing" << std::endl;
            continue;
        }
        
        Material& currentMaterial = materialSystem.GetMaterial(materialID);
        
        for (const auto& reactionEntry : reactions) {
            try {
                std::string reactantName = reactionEntry.at("reactant").get<std::string>();
                std::string product1Name = reactionEntry.at("product1").get<std::string>();
                std::string product2Name = reactionEntry.value("product2", "");

                double probability = reactionEntry.at("probability").get<double>();
                
                // Parse new reaction properties
                std::string typeStr = reactionEntry.value("type", "Contact");
                float speed = reactionEntry.value("speed", 1.0f);
                int range = reactionEntry.value("range", 1);
                bool consumeReactant = reactionEntry.value("consumeReactant", true);
                bool particleEffect = reactionEntry.value("particleEffect", false);

                MaterialID reactantID = materialSystem.GetMaterialID(reactantName);
                MaterialID product1ID = materialSystem.GetMaterialID(product1Name);
                MaterialID product2ID = BGE::MATERIAL_EMPTY;

                // Debug material ID lookup
                std::cout << "DEBUG: Loading reaction for " << materialName << " - reactant '" << reactantName 
                         << "' -> ID " << reactantID << ", product1 '" << product1Name << "' -> ID " << product1ID << std::endl;

                if (reactantID == BGE::MATERIAL_EMPTY && reactantName != "Empty") {
                    std::cerr << "Error: Unknown reactant material '" << reactantName << "' (ID: " << reactantID << ") in reaction for '" << materialName << "'." << std::endl;
                    continue;
                }
                if (product1ID == BGE::MATERIAL_EMPTY && product1Name != "Empty") {
                    std::cerr << "Error: Unknown product1 material '" << product1Name << "' (ID: " << product1ID << ") in reaction for '" << materialName << "'." << std::endl;
                    continue;
                }

                if (!product2Name.empty()) {
                    product2ID = materialSystem.GetMaterialID(product2Name);
                    if (product2ID == BGE::MATERIAL_EMPTY && product2Name != "Empty") {
                        std::cerr << "Warning: Unknown product2 material '" << product2Name << "' (ID: " << product2ID << ") in reaction for '" << materialName << "'. Setting to MATERIAL_EMPTY." << std::endl;
                    }
                }

                // Parse reaction type
                ReactionType reactionType = ReactionType::Contact;
                if (typeStr == "Catalyst") reactionType = ReactionType::Catalyst;
                else if (typeStr == "Dissolve") reactionType = ReactionType::Dissolve;
                else if (typeStr == "Explosive") reactionType = ReactionType::Explosive;
                else if (typeStr == "Corrosive") reactionType = ReactionType::Corrosive;
                else if (typeStr == "Transform") reactionType = ReactionType::Transform;
                else if (typeStr == "Growth") reactionType = ReactionType::Growth;
                else if (typeStr == "Crystallize") reactionType = ReactionType::Crystallize;
                else if (typeStr == "Electrify") reactionType = ReactionType::Electrify;

                BGE::MaterialReaction reaction;
                reaction.reactant = reactantID;
                reaction.product1 = product1ID;
                reaction.product2 = product2ID;
                reaction.type = reactionType;
                reaction.probability = static_cast<float>(probability);
                reaction.speed = speed;
                reaction.range = range;
                reaction.consumeReactant = consumeReactant;
                reaction.particleEffect = particleEffect;

                currentMaterial.AddReaction(reaction);
                std::cout << "Successfully added reaction: " << materialName << " + " << reactantName << " -> " << product1Name;
                if (!product2Name.empty()) std::cout << " + " << product2Name;
                std::cout << std::endl;

            } catch (const nlohmann::json::exception& re) {
                std::cerr << "Error parsing reaction for material '" << materialName << "': " << re.what() << std::endl;
            }
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
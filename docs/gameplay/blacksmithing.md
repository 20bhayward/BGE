# Blacksmithing System Design

## Overview

The blacksmithing system is the core gameplay mechanic of BGE, allowing players to create weapons, tools, and items through realistic material manipulation and forging processes.

## System Architecture

### Core Components

```cpp
// Forging-related components
class ForgeComponent : public Component {
public:
    float temperature = 20.0f;        // Current temperature (Celsius)
    float maxTemperature = 1200.0f;   // Maximum achievable temperature
    float heatRate = 5.0f;            // Degrees per second when active
    float coolRate = 2.0f;            // Degrees per second when cooling
    bool isActive = false;            // Currently heating
    FuelType fuelType = FuelType::COAL;
    float fuelAmount = 0.0f;
};

class AnvilComponent : public Component {
public:
    float durability = 1000.0f;       // Anvil condition
    float efficiency = 1.0f;          // Forging efficiency multiplier
    AnvilType type = AnvilType::IRON;  // Material of the anvil
    Vector2 workSurface = {32, 16};    // Working area in pixels
};

class WorkpieceComponent : public Component {
public:
    MaterialID baseMaterial = MATERIAL_IRON;
    float temperature = 20.0f;
    float malleability = 0.0f;        // 0-1, how workable the material is
    std::vector<ForgeOperation> operations; // History of work done
    ItemShape currentShape = ItemShape::RAW_INGOT;
    Vector2 dimensions = {8, 32};      // Current size
};
```

### Material Properties System

```cpp
struct MaterialProperties {
    // Physical properties
    float meltingPoint = 1000.0f;     // Temperature at which material melts
    float workingTemp = 800.0f;       // Optimal temperature for forging
    float brittleTemp = 200.0f;       // Below this, material becomes brittle
    float density = 7.8f;             // kg/m³
    float hardness = 5.0f;            // Mohs scale
    
    // Forging properties
    float malleabilityRange = 100.0f; // Temperature range for good workability
    float carbonContent = 0.0f;       // For steel alloys (0-2%)
    float quenchability = 0.5f;       // How well it hardens when cooled quickly
    float temperability = 0.8f;       // How well it responds to tempering
    
    // Magical properties (for fantasy elements)
    float magicalConductivity = 0.0f; // How well it conducts magical energy
    float magicalCapacity = 0.0f;     // How much magical energy it can store
    ElementalAffinity affinity = ElementalAffinity::NONE;
    
    // Color and visual
    Color color = Color::SILVER;
    Color glowColor = Color::RED;     // Color when heated
    float emissiveThreshold = 500.0f; // Temperature when it starts glowing
};
```

### Forging Operations

```cpp
enum class ForgeOperation {
    DRAW_OUT,        // Lengthening the material
    UPSET,           // Shortening and thickening
    BEND,            // Creating curves and angles
    TWIST,           // Creating spiral patterns
    PUNCH,           // Creating holes
    FULLER,          // Creating grooves
    SPLIT,           // Separating material
    WELD,            // Joining materials
    QUENCH,          // Rapid cooling for hardening
    TEMPER,          // Controlled reheating for toughness
    POLISH,          // Surface finishing
    SHARPEN          // Edge refinement
};

struct ForgeOperationData {
    ForgeOperation type;
    Vector2 position;        // Where on the workpiece
    float force;             // How hard the blow/operation
    float temperature;       // Temperature when performed
    ToolType tool;           // What tool was used
    float duration;          // How long the operation took
    bool successful;         // Whether it improved or damaged the piece
};
```

### Tool System

```cpp
enum class ToolType {
    // Hammers
    BALL_PEEN_HAMMER,
    CROSS_PEEN_HAMMER,
    STRAIGHT_PEEN_HAMMER,
    SLEDGEHAMMER,
    
    // Tongs
    FLAT_TONGS,
    ROUND_TONGS,
    PICKUP_TONGS,
    
    // Specialized tools
    FULLER,
    CHISEL,
    PUNCH,
    DRIFT,
    SWAGE,
    
    // Files and finishing
    ROUGH_FILE,
    SMOOTH_FILE,
    WHETSTONE,
    GRINDSTONE
};

class ToolComponent : public Component {
public:
    ToolType type;
    float durability = 100.0f;
    float efficiency = 1.0f;          // How effective the tool is
    MaterialID material = MATERIAL_STEEL; // What the tool is made of
    float weight = 1.0f;              // Affects force and stamina usage
    bool isEnchanted = false;         // For magical tools
    
    // Tool-specific properties
    float hammerForce = 10.0f;        // For hammers
    float gripStrength = 5.0f;        // For tongs
    float sharpness = 1.0f;           // For cutting tools
};
```

## Gameplay Mechanics

### 1. Heat Management

```cpp
class ForgeSystem {
public:
    void UpdateForge(ForgeComponent* forge, float deltaTime) {
        if (forge->isActive && forge->fuelAmount > 0) {
            // Heat up
            float heatIncrease = forge->heatRate * deltaTime;
            forge->temperature = std::min(forge->temperature + heatIncrease, 
                                        forge->maxTemperature);
            
            // Consume fuel
            forge->fuelAmount -= CalculateFuelConsumption(forge->temperature) * deltaTime;
            
            // Publish temperature change event
            EventBus::Instance().Publish(ForgeTemperatureChangedEvent{
                forge->GetEntityID(),
                forge->temperature - heatIncrease,
                forge->temperature,
                IsOptimalTemperature(forge->temperature)
            });
        } else {
            // Cool down
            float ambientTemp = 20.0f;
            float cooling = forge->coolRate * deltaTime;
            forge->temperature = std::max(forge->temperature - cooling, ambientTemp);
        }
    }
    
private:
    bool IsOptimalTemperature(float temp) {
        return temp >= 800.0f && temp <= 1000.0f; // Yellow-orange heat
    }
};
```

### 2. Workpiece Manipulation

```cpp
class BlacksmithingSystem {
public:
    bool PerformOperation(Entity* workpiece, Entity* tool, Entity* anvil, 
                         ForgeOperation operation, Vector2 position, float force) {
        auto workpieceComp = workpiece->GetComponent<WorkpieceComponent>();
        auto toolComp = tool->GetComponent<ToolComponent>();
        auto anvilComp = anvil->GetComponent<AnvilComponent>();
        
        if (!workpieceComp || !toolComp || !anvilComp) return false;
        
        // Check if material is workable
        if (!IsMaterialWorkable(workpieceComp)) {
            BGE_LOG_WARNING("Blacksmithing", "Material too cold or too hot to work");
            return false;
        }
        
        // Calculate operation success
        float successChance = CalculateSuccessChance(workpieceComp, toolComp, 
                                                   anvilComp, operation, force);
        
        bool success = (Random::Float() < successChance);
        
        // Apply operation effects
        if (success) {
            ApplySuccessfulOperation(workpieceComp, operation, position, force);
        } else {
            ApplyFailedOperation(workpieceComp, operation);
        }
        
        // Record operation in history
        workpieceComp->operations.push_back({
            operation, position, force, workpieceComp->temperature,
            toolComp->type, GetOperationDuration(operation), success
        });
        
        // Reduce material temperature from work
        workpieceComp->temperature -= CalculateHeatLoss(operation, force);
        
        // Reduce tool durability
        toolComp->durability -= CalculateToolWear(operation, force, workpieceComp->baseMaterial);
        
        // Publish crafting event
        EventBus::Instance().Publish(ForgingOperationEvent{
            workpiece->GetID(), operation, success, workpieceComp->temperature
        });
        
        return success;
    }
    
private:
    bool IsMaterialWorkable(WorkpieceComponent* workpiece) {
        auto material = MaterialDatabase::GetMaterial(workpiece->baseMaterial);
        float temp = workpiece->temperature;
        
        return temp >= material.workingTemp - material.malleabilityRange/2 &&
               temp <= material.workingTemp + material.malleabilityRange/2 &&
               temp < material.meltingPoint;
    }
    
    float CalculateSuccessChance(WorkpieceComponent* workpiece, ToolComponent* tool,
                                AnvilComponent* anvil, ForgeOperation operation, float force) {
        float baseChance = 0.7f;
        
        // Temperature factor
        auto material = MaterialDatabase::GetMaterial(workpiece->baseMaterial);
        float tempDiff = std::abs(workpiece->temperature - material.workingTemp);
        float tempFactor = 1.0f - (tempDiff / material.malleabilityRange);
        
        // Tool efficiency
        float toolFactor = tool->efficiency * (tool->durability / 100.0f);
        
        // Anvil quality
        float anvilFactor = anvil->efficiency;
        
        // Force factor (too much or too little is bad)
        float optimalForce = GetOptimalForce(operation, workpiece->baseMaterial);
        float forceDiff = std::abs(force - optimalForce) / optimalForce;
        float forceFactor = 1.0f - std::min(forceDiff, 1.0f);
        
        return baseChance * tempFactor * toolFactor * anvilFactor * forceFactor;
    }
};
```

### 3. Recipe and Pattern System

```cpp
struct CraftingRecipe {
    std::string name;
    ItemType resultType;
    
    // Required materials
    std::vector<MaterialRequirement> materials;
    
    // Required operations in sequence
    std::vector<OperationStep> steps;
    
    // Tolerances for each step
    std::vector<OperationTolerance> tolerances;
    
    // Difficulty factors
    float temperaturePrecision = 50.0f;  // ±50°C tolerance
    float positionPrecision = 2.0f;      // ±2 pixel tolerance
    float forcePrecision = 0.2f;         // ±20% force tolerance
    
    // Quality factors
    QualityRequirements quality;
};

struct OperationStep {
    ForgeOperation operation;
    Vector2 targetPosition;
    float targetForce;
    float targetTemperature;
    ToolType requiredTool;
    float timeLimit = 0.0f;  // 0 = no limit
};

class RecipeSystem {
public:
    bool LoadRecipes(const std::string& recipeFile) {
        // Load from JSON/XML data files
        auto recipes = JsonLoader::LoadArray(recipeFile);
        
        for (const auto& recipeData : recipes) {
            CraftingRecipe recipe;
            recipe.name = recipeData["name"];
            recipe.resultType = ParseItemType(recipeData["result"]);
            
            // Parse materials
            for (const auto& matData : recipeData["materials"]) {
                recipe.materials.push_back({
                    ParseMaterialID(matData["type"]),
                    matData["amount"],
                    matData["purity"]
                });
            }
            
            // Parse steps
            for (const auto& stepData : recipeData["steps"]) {
                recipe.steps.push_back({
                    ParseOperation(stepData["operation"]),
                    {stepData["x"], stepData["y"]},
                    stepData["force"],
                    stepData["temperature"],
                    ParseToolType(stepData["tool"])
                });
            }
            
            m_recipes[recipe.name] = recipe;
        }
        
        return true;
    }
    
    RecipeMatchResult CheckRecipeMatch(const std::vector<ForgeOperationData>& operations,
                                     const WorkpieceComponent* workpiece) {
        for (const auto& [name, recipe] : m_recipes) {
            float matchScore = CalculateMatchScore(operations, recipe);
            if (matchScore >= 0.8f) { // 80% match required
                return {true, name, matchScore, CalculateQuality(operations, recipe)};
            }
        }
        return {false, "", 0.0f, ItemQuality::POOR};
    }
    
private:
    std::unordered_map<std::string, CraftingRecipe> m_recipes;
};
```

### 4. Quality System

```cpp
enum class ItemQuality {
    POOR = 0,      // 0-20% recipe match
    COMMON = 1,    // 20-40% recipe match  
    GOOD = 2,      // 40-60% recipe match
    EXCELLENT = 3, // 60-80% recipe match
    MASTERWORK = 4, // 80-95% recipe match
    LEGENDARY = 5   // 95%+ recipe match + special conditions
};

struct ItemProperties {
    float durability = 100.0f;
    float sharpness = 1.0f;      // For bladed weapons
    float balance = 0.5f;        // 0=handle heavy, 1=blade heavy
    float flexibility = 0.5f;    // 0=brittle, 1=flexible
    float weight = 1.0f;
    
    // Combat properties
    float damage = 10.0f;
    float criticalChance = 0.05f;
    float armorPenetration = 0.0f;
    
    // Magical properties
    float manaCapacity = 0.0f;
    std::vector<Enchantment> enchantments;
};

class QualitySystem {
public:
    ItemProperties CalculateItemProperties(const CraftingRecipe& recipe,
                                         const std::vector<ForgeOperationData>& operations,
                                         ItemQuality quality) {
        ItemProperties props;
        
        // Base properties from recipe
        props = GetBaseProperties(recipe.resultType);
        
        // Apply quality multipliers
        float qualityMultiplier = GetQualityMultiplier(quality);
        props.durability *= qualityMultiplier;
        props.damage *= qualityMultiplier;
        
        // Analyze forging technique
        AnalyzeForgingTechnique(operations, props);
        
        // Apply material bonuses
        ApplyMaterialBonuses(recipe.materials, props);
        
        return props;
    }
    
private:
    void AnalyzeForgingTechnique(const std::vector<ForgeOperationData>& operations,
                                ItemProperties& props) {
        // Analyze operation sequence for technique quality
        float temperatureConsistency = CalculateTemperatureConsistency(operations);
        float operationPrecision = CalculateOperationPrecision(operations);
        
        // Good temperature control improves durability
        props.durability *= (0.5f + 0.5f * temperatureConsistency);
        
        // Precise operations improve balance and sharpness
        props.balance = std::clamp(props.balance + (operationPrecision - 0.5f), 0.0f, 1.0f);
        props.sharpness *= (0.5f + 0.5f * operationPrecision);
    }
};
```

## Events and Integration

### Blacksmithing Events

```cpp
struct ForgeOperationEvent {
    EntityID workpieceID;
    ForgeOperation operation;
    bool successful;
    float temperature;
    Vector2 position;
    float force;
};

struct ItemCompletedEvent {
    EntityID crafterID;
    EntityID workpieceID;
    std::string recipeName;
    ItemQuality quality;
    ItemProperties properties;
    float craftingTime;
};

struct ToolBrokenEvent {
    EntityID toolID;
    ToolType type;
    EntityID userID;
};

struct RecipeDiscoveredEvent {
    EntityID playerID;
    std::string recipeName;
    float discoveryScore; // How close they got to the "official" recipe
};
```

### Integration with Other Systems

```cpp
// Physics Integration
EventBus::Instance().Subscribe<ForgeOperationEvent>([](const ForgeOperationEvent& event) {
    // Generate sparks and particles
    auto sparks = ParticleSystem::CreateSparks(event.position, event.force);
    
    // Play hammer sound
    AudioSystem::PlaySpatialSound("hammer_strike.wav", event.position, 
                                 event.force / 10.0f); // Volume based on force
    
    // Screen shake based on force
    CameraSystem::AddShake(event.force * 0.1f);
});

// Experience System Integration  
EventBus::Instance().Subscribe<ItemCompletedEvent>([](const ItemCompletedEvent& event) {
    auto player = EntityManager::Instance().GetEntity(event.crafterID);
    if (auto exp = player->GetComponent<ExperienceComponent>()) {
        float xpGain = CalculateXPGain(event.quality, event.craftingTime);
        exp->GainExperience(SkillType::BLACKSMITHING, xpGain);
    }
});
```

## Future Enhancements

### Advanced Features
1. **Alloy System**: Mixing different metals for unique properties
2. **Pattern Welding**: Complex layered metal techniques  
3. **Magical Forging**: Incorporating magical materials and processes
4. **Apprentice System**: Teaching NPCs your discovered techniques
5. **Market Economics**: Supply and demand affecting material prices
6. **Historical Techniques**: Unlocking period-accurate methods

### Data-Driven Configuration
```json
{
  "recipes": [
    {
      "name": "Iron Sword",
      "result": "SWORD_IRON",
      "materials": [
        {"type": "IRON_INGOT", "amount": 2, "purity": 0.8}
      ],
      "steps": [
        {
          "operation": "DRAW_OUT",
          "position": [16, 8],
          "force": 8.0,
          "temperature": 900,
          "tool": "CROSS_PEEN_HAMMER"
        },
        {
          "operation": "FULLER",
          "position": [16, 4],
          "force": 6.0,
          "temperature": 850,
          "tool": "FULLER"
        }
      ],
      "tolerances": {
        "temperature": 75,
        "position": 3,
        "force": 0.25
      }
    }
  ]
}
```
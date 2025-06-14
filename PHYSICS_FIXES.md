# Powder Physics Density Fixes

## Changes Made

### Problem
The powder physics system was not correctly implementing density-based interactions. Materials were falling through each other incorrectly, ignoring realistic density relationships.

### Key Issues Fixed

1. **Liquid-Powder Interactions**: 
   - **Before**: Liquids could displace powders "regardless of density"
   - **After**: Only denser liquids can displace lighter powders
   - **Result**: Water (1.0) no longer displaces sand (1.6), sand sinks in water

2. **Powder-Liquid Interactions**:
   - **Before**: No specific rules for powders falling into liquids
   - **After**: Dense powders sink through lighter liquids, light powders float on dense liquids
   - **Result**: Sand (1.6) sinks through water (1.0) but floats on molten iron (7.8)

3. **Oil Buoyancy Logic**:
   - **Before**: Hardcoded to only check for material named "Water"
   - **After**: Uses density comparison with any liquid
   - **Result**: Oil properly floats on any denser liquid, not just water

4. **Powder-Powder Displacement**:
   - **Before**: Required density difference > 0.2f
   - **After**: Reduced threshold to 0.1f for more responsive physics

### Material Scope
**30 core materials with focused density-based interactions:**
- **7 powder materials** ranging from 0.3 (Snow) to 1.7 (Clay)
- **6 liquid materials** ranging from 0.8 (Oil) to 3.0 (Lava)  
- **7 gas materials** ranging from 0.07 (Hydrogen) to 1.1 (Oxygen)
- **10 static materials** ranging from 0.7 (Wood) to 7.8 (Metal)

### Key Material Densities (Core Set)
**Ultra-light materials (< 0.5):**
- Hydrogen: 0.07, Nitrogen: 0.2, Snow: 0.3, Steam: 0.3, Dust: 0.4

**Light materials (0.5 - 1.0):**
- Ash: 0.5, Fire: 0.5, Wood: 0.7, Oil: 0.8, Ice: 0.92

**Medium materials (1.0 - 2.0):**  
- Water: 1.0, Gunpowder: 1.2, Dirt: 1.3, Sand: 1.6, Clay: 1.7

**Heavy materials (2.0+):**
- Stone: 2.7, Lava: 3.0, Diamond: 3.5, Metal: 7.8

### Expected Behaviors (Core Materials)
✅ Light powders (Snow, Dust, Ash) float on most liquids
✅ Medium powders (Sand, Dirt, Clay) sink in light liquids but float on Lava
✅ All gases rise through liquids and powders  
✅ Oil floats on Water; both float on Lava
✅ Proper density stratification in complex scenarios
✅ Static materials provide stable foundations

### Files Modified
- `/Simulation/CellularAutomata.cpp`:
  - `TryMove()` function: Fixed liquid-powder interaction rules
  - `TryPowderMove()` function: Improved powder displacement logic
  - `ProcessOil()` function: Replaced hardcoded water check with density logic

## Core Material Interaction Examples

### Realistic Density Layering
1. **Light powder floating**: Drop Snow (0.3) or Dust (0.4) into Water - should float on surface
2. **Medium powder sinking**: Drop Sand (1.6) or Dirt (1.3) into Water - should sink through
3. **Heavy liquid displacement**: Pour Lava (3.0) onto Sand - Lava should displace and settle below
4. **Gas rising**: All gases should rise through any liquid or powder materials
5. **Oil-water separation**: Oil (0.8) naturally floats on Water (1.0)

### Complex Scenarios  
- **Layered density**: Create Oil → Water → Sand → Stone layers (lightest to heaviest)
- **Buoyancy**: Light materials like Ash (0.5) float on dense liquids like Magma (2.8)
- **Realistic settling**: Materials automatically sort by density in still pools

### Reaction Products
- Fire reactions produce appropriate densities: Ash (0.5) floats, Smoke (0.4) rises
- Lava cooling with LiquidNitrogen creates Stone (2.7) that sinks properly
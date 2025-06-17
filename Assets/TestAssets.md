# Test Assets for Enhanced Asset System

## Created Assets

### Materials
- `Materials/SandMaterial.json` - Sandy brown material with high roughness
- `Materials/WaterMaterial.json` - Blue translucent material with low roughness  
- `Materials/FireMaterial.json` - Red-orange emissive material

### Scenes
- `Scenes/DemoScene.json` - Basic scene with camera and directional light

### Images
- `Images/test_sprite.png` - Existing test sprite

## Expected .meta Files

The AssetRegistry should automatically generate:
- `Materials/SandMaterial.json.meta`
- `Materials/WaterMaterial.json.meta` 
- `Materials/FireMaterial.json.meta`
- `Scenes/DemoScene.json.meta`
- `Images/test_sprite.png.meta`

Each .meta file contains:
- Unique UUID for persistent asset referencing
- Asset type information
- Import settings
- Version information

## Testing the System

1. **Launch InteractiveEditor**
   - The Enhanced Asset Browser should show all assets with visual icons
   - Assets should be organized in a directory tree
   
2. **Asset Selection**
   - Click on any material in the Asset Browser
   - The Asset Inspector should show editable material properties
   
3. **Live Editing**
   - Modify material properties in the Asset Inspector
   - Changes should auto-save and trigger hot-reloading
   
4. **Hot-Reloading**
   - Edit material files externally while the editor is running
   - Changes should be detected and reflected immediately

5. **UUID System**
   - Check that .meta files are generated automatically
   - Verify that asset references use UUIDs instead of file paths
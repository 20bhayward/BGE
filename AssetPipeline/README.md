# BGE Enhanced Asset Pipeline

## Overview

The BGE Enhanced Asset Pipeline provides a professional, Unity-like asset management system with UUID-based asset referencing, hot-reloading, and automated .meta file generation.

## Key Features

### 🎯 UUID-based Asset System
- Every asset gets a persistent UUID stored in `.meta` files
- Assets can be moved/renamed without breaking references
- Dependency tracking between assets

### 🔥 Hot-Reloading
- Automatic detection of file system changes
- Live asset updates without engine restart
- Asset reload events for UI synchronization

### 📁 Asset Types Supported
- **Textures**: .png, .jpg, .jpeg, .bmp, .tga
- **Materials**: .json material definitions
- **Prefabs**: .bprefab entity templates
- **Scenes**: .json scene files
- **Audio**: .wav, .mp3, .ogg (future)
- **Scripts**: .cpp, .h files (future)
- **Models**: .obj, .fbx (future)

### 🎨 Enhanced Asset Browser
- Unity-style directory tree navigation
- Asset grid with visual thumbnails
- Search and filtering capabilities
- Context menus for asset operations
- Drag-and-drop support

### 🔧 Asset Inspector
- Live asset editing with immediate feedback
- Material property editing with real-time preview
- Texture import settings
- Asset metadata display

## Architecture

### Core Components

1. **AssetHandle**: UUID-based asset identification
2. **AssetRegistry**: Manages .meta files and asset lookup
3. **AssetManager**: Central hub for asset loading and caching
4. **IAssetLoader**: Extensible loader system for different asset types
5. **IAsset**: Base interface for all asset types

### Asset Loading Pipeline

```
File System → AssetRegistry → AssetLoader → AssetManager → Cache
     ↓              ↓              ↓            ↓         ↓
  .meta file    UUID lookup    Type-specific   Generic    Fast
  generation                     loading      caching   access
```

## Usage

### Basic Asset Loading

```cpp
// Get asset manager
auto assetManager = Services::GetAssets();

// Load a texture by path
AssetHandle handle = assetManager->LoadAsset("Assets/Images/sprite.png");

// Get the texture asset
auto texture = assetManager->GetTexture(handle);
if (texture) {
    // Use texture->rendererId for rendering
}
```

### Material Editing

```cpp
// Load a material
auto material = assetManager->GetMaterial(materialHandle);

// Modify properties
material->data.color[0] = 1.0f; // Red
material->data.roughness = 0.5f;

// Material will be auto-saved by the Asset Inspector UI
```

### Hot-Reloading Events

```cpp
// Subscribe to asset reload events
eventBus->Subscribe<AssetReloadedEvent>([](const AssetReloadedEvent& event) {
    std::cout << "Asset reloaded: " << event.path << std::endl;
    // Update UI, invalidate caches, etc.
});
```

## Integration with InteractiveEditor

The InteractiveEditor now showcases the enhanced asset system:

1. **Enhanced Asset Browser**: Browse and manage assets with visual thumbnails
2. **Asset Inspector**: Edit material properties with live preview
3. **Hot-Reloading**: Modify asset files externally and see changes immediately
4. **UUID System**: All assets get persistent IDs for reliable referencing

## File Structure

```
Assets/
├── Materials/
│   ├── SandMaterial.json
│   ├── SandMaterial.json.meta
│   └── WaterMaterial.json
├── Images/
│   ├── test_sprite.png
│   └── test_sprite.png.meta
└── Scenes/
    └── DemoScene.json
```

Each asset file automatically gets a corresponding `.meta` file containing:
- Unique UUID
- Asset type information
- Import settings
- Version information

## Future Enhancements

- **Prefab System**: Complete entity serialization and instantiation
- **Drag-and-Drop**: Full drag-and-drop workflow from browser to scene
- **Advanced Thumbnails**: 3D model previews, material spheres
- **Asset Import Pipeline**: Custom importers with settings
- **Asset Dependencies**: Automatic dependency resolution
- **Asset Streaming**: Large world asset loading optimization
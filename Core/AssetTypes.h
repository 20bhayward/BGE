#pragma once

#include <string>

namespace BGE {

// Asset type enumeration
enum class AssetType {
    Unknown,
    Folder,
    Texture,        // .png, .jpg, .jpeg, .bmp, .tga
    Material,       // .json (material definitions)
    Scene,          // .json (scene files)
    Audio,          // .wav, .mp3, .ogg
    Script,         // .cpp, .h
    Prefab,         // .bprefab (BGE prefab files)
    Model,          // .obj, .fbx (future)
    Animation       // .anim (future)
};

// Asset selection event for Inspector integration
struct AssetSelectionChangedEvent {
    std::string selectedAssetPath;
    AssetType selectedAssetType;
    
    AssetSelectionChangedEvent() = default;
    AssetSelectionChangedEvent(const std::string& path, AssetType type) 
        : selectedAssetPath(path), selectedAssetType(type) {}
};

} // namespace BGE
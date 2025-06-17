# BGE Asset Pipeline
# UUID-based asset management system with hot-reloading

add_library(BGEAssetPipeline STATIC
    AssetHandle.h
    AssetHandle.cpp
    IAsset.h
    AssetRegistry.h
    AssetRegistry.cpp
    IAssetLoader.h
    IAssetLoader.cpp
    AssetManager.h
    AssetManager.cpp
)

target_include_directories(BGEAssetPipeline PUBLIC .)
target_link_libraries(BGEAssetPipeline PUBLIC BGECore BGERenderer)
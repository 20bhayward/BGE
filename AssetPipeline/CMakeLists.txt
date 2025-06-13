# BGE Asset Pipeline
# Currently minimal implementation

add_library(BGEAssetPipeline STATIC
    # Placeholder files - to be implemented
    AssetManager.h
    AssetManager.cpp
)

target_include_directories(BGEAssetPipeline PUBLIC .)
target_link_libraries(BGEAssetPipeline PUBLIC BGECore)

# TODO: Add asset loading libraries when implemented
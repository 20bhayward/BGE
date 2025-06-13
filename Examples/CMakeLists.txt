add_subdirectory(BasicSandbox)
add_subdirectory(InteractiveEditor)

# Additional examples
if(BGE_BUILD_EXAMPLES)
    add_subdirectory(LightingDemo)
    add_subdirectory(MaterialEditor)
    add_subdirectory(PhysicsDemo)
    add_subdirectory(ProceduralWorld)
    add_subdirectory(HotReloadingTest)
endif()
add_executable(InteractiveEditor
    main.cpp
)

target_link_libraries(InteractiveEditor
    BGECore
    BGESimulation
    BGERenderer
    BGEAI
    BGEAssetPipeline
)

# Copy executable to build output
set_target_properties(InteractiveEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples/InteractiveEditor"
)
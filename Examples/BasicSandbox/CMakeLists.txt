add_executable(BasicSandbox
    main.cpp
)

target_link_libraries(BasicSandbox PRIVATE
    BGECore
    BGESimulation
    BGERenderer
)

# Copy assets to build directory
file(GLOB_RECURSE ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*")
if(ASSET_FILES)
    foreach(ASSET ${ASSET_FILES})
        file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${ASSET})
        configure_file(${ASSET} ${CMAKE_CURRENT_BINARY_DIR}/${REL_PATH} COPYONLY)
    endforeach()
endif()

# Set working directory for debugging
set_target_properties(BasicSandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
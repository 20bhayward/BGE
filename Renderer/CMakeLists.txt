add_library(BGERenderer STATIC
    # Core rendering (only include files that exist)
    Renderer.h
    Renderer.cpp
    RenderDevice.h
    RenderDevice.cpp
    
    # Lighting system
    Lighting/LightingSystem.h
    
    # Graphics API abstraction
    GraphicsAPI.h
    CommandBuffer.h
    Pipeline.h
    
    # Vulkan backend (minimal)
    Vulkan/VulkanRenderer.h
    Vulkan/VulkanRenderer.cpp
    Vulkan/VulkanDevice.h
    Vulkan/VulkanDevice.cpp
    Vulkan/VulkanCommandBuffer.h
    Vulkan/VulkanCommandBuffer.cpp
    
    # TODO: Add remaining files as they are implemented
    # Lighting/Raytracer2D.h
    # Lighting/Light.h
)

target_include_directories(BGERenderer PUBLIC .)
target_link_libraries(BGERenderer PUBLIC BGECore BGESimulation)

# Vulkan support
if(BGE_USE_VULKAN AND Vulkan_FOUND)
    target_link_libraries(BGERenderer PUBLIC Vulkan::Vulkan)
    target_compile_definitions(BGERenderer PUBLIC BGE_VULKAN_SUPPORT)
    
    # Shader compilation
    find_program(GLSLC_EXECUTABLE glslc HINTS ${Vulkan_GLSLC_EXECUTABLE})
    if(GLSLC_EXECUTABLE)
        target_compile_definitions(BGERenderer PRIVATE BGE_SHADER_COMPILATION)
    endif()
endif()

# OpenGL support
if(BGE_USE_OPENGL AND OpenGL_FOUND)
    target_link_libraries(BGERenderer PUBLIC OpenGL::GL)
    target_compile_definitions(BGERenderer PUBLIC BGE_OPENGL_SUPPORT)
    
    # Try to find GLAD via vcpkg first
    find_package(glad CONFIG QUIET)
    if(glad_FOUND)
        target_link_libraries(BGERenderer PUBLIC glad::glad)
        message(STATUS "Using vcpkg GLAD")
    else()
        # Check if our GLAD directory exists
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/glad)
            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/glad ${CMAKE_CURRENT_BINARY_DIR}/glad)
            target_link_libraries(BGERenderer PUBLIC glad)
            message(STATUS "Using local GLAD")
        else()
            message(WARNING "GLAD not found. OpenGL functionality may be limited.")
            message(STATUS "To fix: install via vcpkg: vcpkg install glad:x64-windows")
        endif()
    endif()
endif()

# Shader embedding
file(GLOB_RECURSE SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert" 
                               "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag"
                               "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.comp"
                               "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.glsl")

if(SHADER_FILES)
    add_custom_target(CompileShaders ALL)
    foreach(SHADER ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Shaders/${SHADER_NAME}.spv)
        
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Shaders
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling shader ${SHADER_NAME}"
        )
        
        add_custom_target(${SHADER_NAME}_compile DEPENDS ${SHADER_OUTPUT})
        add_dependencies(CompileShaders ${SHADER_NAME}_compile)
    endforeach()
    
    add_dependencies(BGERenderer CompileShaders)
endif()
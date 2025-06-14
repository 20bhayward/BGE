cmake_minimum_required(VERSION 3.20)
project(BGE VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Engine configuration
option(BGE_BUILD_EXAMPLES "Build BGE examples" ON)
option(BGE_BUILD_EDITOR "Build BGE editor" ON)
option(BGE_BUILD_TESTS "Build BGE tests" ON)
option(BGE_USE_VULKAN "Use Vulkan renderer" ON)
option(BGE_USE_OPENGL "Use OpenGL renderer fallback" ON)

# Platform detection
if(WIN32)
    set(BGE_PLATFORM "Windows")
elseif(UNIX AND NOT APPLE)
    set(BGE_PLATFORM "Linux")
elseif(APPLE)
    set(BGE_PLATFORM "macOS")
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native)
    endif()
endif()

# Find packages
# Help CMake find Vulkan - check multiple possible locations
set(VULKAN_SEARCH_PATHS
    "G:/Vulkan"
    "C:/VulkanSDK"
    "$ENV{VULKAN_SDK}"
    "$ENV{VK_SDK_PATH}"
)

foreach(VULKAN_PATH ${VULKAN_SEARCH_PATHS})
    if(EXISTS ${VULKAN_PATH})
        set(VULKAN_SDK_PATH ${VULKAN_PATH})
        break()
    endif()
endforeach()

if(VULKAN_SDK_PATH)
    # Try different library naming conventions
    find_library(Vulkan_LIBRARY 
        NAMES vulkan-1 vulkan
        PATHS ${VULKAN_SDK_PATH}/Lib ${VULKAN_SDK_PATH}/lib
        NO_DEFAULT_PATH
    )
    find_path(Vulkan_INCLUDE_DIR 
        NAMES vulkan/vulkan.h
        PATHS ${VULKAN_SDK_PATH}/Include ${VULKAN_SDK_PATH}/include
        NO_DEFAULT_PATH
    )
    if(Vulkan_LIBRARY AND Vulkan_INCLUDE_DIR)
        message(STATUS "Found Vulkan SDK at: ${VULKAN_SDK_PATH}")
        message(STATUS "Vulkan Library: ${Vulkan_LIBRARY}")
        message(STATUS "Vulkan Include: ${Vulkan_INCLUDE_DIR}")
    endif()
endif()

find_package(Vulkan)
find_package(OpenGL)

# Handle threading - suppress pthread warnings on Windows
if(WIN32)
    find_package(Threads REQUIRED)
    # Windows uses Win32 threads, not pthreads - warnings are expected
else()
    find_package(Threads REQUIRED)
endif()

# Find GLFW3 with better error handling
find_package(glfw3 QUIET CONFIG)
if(NOT glfw3_FOUND)
    # Try alternative names and methods
    find_package(glfw QUIET CONFIG)
    if(NOT glfw_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(GLFW3 QUIET glfw3)
        endif()
        
        if(NOT GLFW3_FOUND)
            message(STATUS "GLFW3 not found via package managers, looking for system installation...")
            find_library(GLFW_LIBRARY NAMES glfw3 glfw glfw3dll
                PATHS 
                    "G:/vcpkg/installed/x64-windows/lib"
                    "C:/vcpkg/installed/x64-windows/lib"
                    "${CMAKE_PREFIX_PATH}/lib"
                PATH_SUFFIXES lib)
            find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h
                PATHS 
                    "G:/vcpkg/installed/x64-windows/include"
                    "C:/vcpkg/installed/x64-windows/include"
                    "${CMAKE_PREFIX_PATH}/include"
                PATH_SUFFIXES include)
            
            if(GLFW_LIBRARY AND GLFW_INCLUDE_DIR)
                message(STATUS "Found GLFW manually: ${GLFW_LIBRARY}")
                add_library(glfw UNKNOWN IMPORTED)
                set_target_properties(glfw PROPERTIES
                    IMPORTED_LOCATION "${GLFW_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${GLFW_INCLUDE_DIR}")
            else()
                message(WARNING "GLFW3 not found. Trying to fetch from source...")
                set(GLFW_FETCH_FROM_SOURCE ON)
            endif()
        else()
            # Create target from pkg-config
            add_library(glfw INTERFACE)
            target_link_libraries(glfw INTERFACE ${GLFW3_LIBRARIES})
            target_include_directories(glfw INTERFACE ${GLFW3_INCLUDE_DIRS})
            target_compile_options(glfw INTERFACE ${GLFW3_CFLAGS_OTHER})
        endif()
    else()
        # glfw target already exists, no alias needed
        message(STATUS "Using existing glfw target")
    endif()
else()
    # glfw3 found, create alias if glfw doesn't exist
    if(NOT TARGET glfw)
        add_library(glfw ALIAS glfw3)
    endif()
endif()

# Third party libraries
add_subdirectory(ThirdParty)

# Engine modules
add_subdirectory(Core)
add_subdirectory(Simulation)
add_subdirectory(Renderer)
add_subdirectory(Audio)
add_subdirectory(AI)
add_subdirectory(Scripting)
add_subdirectory(AssetPipeline)
add_subdirectory(Runtime)

# Editor
if(BGE_BUILD_EDITOR)
    add_subdirectory(Editor)
endif()

# Examples
if(BGE_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

# Tests
if(BGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
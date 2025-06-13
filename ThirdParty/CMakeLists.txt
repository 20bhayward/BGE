# Third party libraries

# Only fetch GLFW if requested
if(GLFW_FETCH_FROM_SOURCE)
    message(STATUS "Fetching GLFW from source as fallback...")
    
    include(FetchContent)
    
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )
    
    # Configure GLFW options
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(glfw)
    
    message(STATUS "GLFW fetched and configured successfully")
endif()

# Add any other third-party libraries here in the future
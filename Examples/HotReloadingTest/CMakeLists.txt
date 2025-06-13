cmake_minimum_required(VERSION 3.10)
project(HotReloadingTest)

# Find BGE; this assumes BGE is built as a library or is easily findable
# This part is highly dependent on the main project's CMake structure.
# Assuming BGE is a target from the parent scope.
# If BGE is an installed library, you might use find_package(BGE REQUIRED)

add_executable(HotReloadingTest main.cpp)

# Link against BGE libraries. This needs to match how BGE components are named.
# Common practice: a single BGE::Core or multiple BGE::AssetPipeline, BGE::Renderer etc.
# For this example, let's assume a general BGE target or list essential ones.
# This list might need adjustment based on actual target names in BGE's CMake.
target_link_libraries(HotReloadingTest
    PRIVATE
    BGECore        # For Engine, Application, ServiceLocator, Logger
    BGEAssetPipeline # For AssetManager
    BGERenderer    # For Texture, Renderer (even if placeholders)
    # Add other BGE modules if direct dependencies arise
)

# Add include directories for BGE headers
# Assuming BGE_SOURCE_DIR is the root of the BGE project
target_include_directories(HotReloadingTest PRIVATE ${CMAKE_SOURCE_DIR})

# Copy Assets directory to build output (if needed for the executable to find them)
# This path needs to be relative to this CMakeLists.txt
set(ASSETS_DIR_SOURCE ${CMAKE_SOURCE_DIR}/Assets)
set(ASSETS_DIR_DEST ${CMAKE_CURRENT_BINARY_DIR}/Assets)

# This command might be too broad; ideally, copy only needed assets.
# For the test_sprite.png specifically:
set(TEST_IMAGE_SOURCE ${CMAKE_SOURCE_DIR}/Assets/Images/test_sprite.png)
set(TEST_IMAGE_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/Assets/Images)

add_custom_command(
    TARGET HotReloadingTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_IMAGE_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TEST_IMAGE_SOURCE} ${TEST_IMAGE_DEST_DIR}/
    COMMENT "Copying test_sprite.png to build directory"
)

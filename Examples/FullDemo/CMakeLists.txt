# BGE Full Demo Application
cmake_minimum_required(VERSION 3.10)

add_executable(FullDemo main.cpp)

# Link against all BGE libraries
target_link_libraries(FullDemo PRIVATE
    BGECore
    BGESimulation
    BGERenderer
    BGEAI
    BGEAudio
    BGEAssetPipeline
)

# Include directories
target_include_directories(FullDemo PRIVATE ../../)
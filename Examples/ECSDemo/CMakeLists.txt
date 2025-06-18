add_executable(ECSDemo main.cpp)

target_link_libraries(ECSDemo PRIVATE BGECore)

# Include directories
target_include_directories(ECSDemo PRIVATE 
    ${CMAKE_SOURCE_DIR}
)

# Set output directory
set_target_properties(ECSDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
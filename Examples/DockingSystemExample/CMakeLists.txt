project(DockingSystemExample)

add_executable(DockingSystemExample main.cpp)

target_link_libraries(DockingSystemExample 
    BGECore 
    BGERenderer 
    BGESimulation
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(DockingSystemExample gdi32 opengl32)
elseif(UNIX)
    target_link_libraries(DockingSystemExample GL X11)
endif()

# Copy configuration file
configure_file(../../config.ini config.ini COPYONLY)
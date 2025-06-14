add_library(BGECore STATIC
    # Foundation
    Engine.cpp
    Engine.h
    Application.cpp
    Application.h
    ServiceLocator.cpp
    ServiceLocator.h
    EventBus.cpp
    EventBus.h
    Events.h
    Logger.cpp
    Logger.h
    Entity.cpp
    Entity.h
    ConfigManager.cpp
    ConfigManager.h
    Services.h
    Services.cpp
    Components.h
    Components.cpp
    
    # Enhanced ECS System
    Systems/ISystem.h
    Systems/SystemManager.h
    Systems/SystemManager.cpp
    Systems/TransformSystem.h
    Systems/TransformSystem.cpp
    
    # ECS Components and Queries
    ECS/Archetype.h
    ECS/EntityQuery.h
    ECS/EntityQuery.cpp
    
    # Math
    Math/Vector2.h
    Math/Vector3.h
    Math/Vector4.h
    Math/Matrix4.h
    Math/Math.h
    Math/Math.cpp
    
    # Memory
    Memory/MemoryPool.h
    Memory/MemoryPool.cpp
    Memory/Allocator.h
    Memory/Allocator.cpp
    
    # Threading
    Threading/ThreadPool.h
    Threading/ThreadPool.cpp
    Threading/JobSystem.h
    Threading/JobSystem.cpp
    
    # Time
    Time/Timer.h
    Time/Timer.cpp
    Time/Clock.h
    Time/Clock.cpp
    
    # Input
    Input/InputManager.h
    Input/InputManager.cpp
    Input/Keyboard.h
    Input/Keyboard.cpp
    Input/Mouse.h
    Input/Mouse.cpp
    Input/MaterialBrush.h
    Input/MaterialBrush.cpp
    Input/MaterialPalette.h
    Input/MaterialPalette.cpp
    Input/MaterialTools.h
    Input/MaterialTools.cpp
    Input/CategorizedMaterialPalette.h
    Input/CategorizedMaterialPalette.cpp
    
    # UI System
    UI/UISystem.h
    UI/UISystem.cpp
    UI/MaterialEditorUI.h
    UI/MaterialEditorUI.cpp
    UI/Panel.h
    UI/PanelManager.h
    UI/LayoutInfo.h
    UI/TabbedPanel.h
    UI/TabbedPanel.cpp
    UI/UnityLayoutManager.h
    UI/UnityLayoutManager.cpp
    UI/AssetBrowserPanel.h
    UI/AssetBrowserPanel.cpp
    UI/SceneViewPanel.h
    UI/SceneViewPanel.cpp
    UI/GameViewportPanel.h
    UI/GameViewportPanel.cpp
    UI/InspectorPanel.h
    UI/InspectorPanel.cpp
    UI/HierarchyPanel.h
    UI/HierarchyPanel.cpp
    UI/MaterialPalettePanel.h
    UI/MaterialPalettePanel.cpp
    UI/DebugToolbarPanel.h
    UI/DebugToolbarPanel.cpp
    
    # Platform
    Platform/Platform.h
    Platform/Window.h
)

target_include_directories(BGECore PUBLIC .)

# Find ImGui
find_package(imgui CONFIG REQUIRED)

# Find OpenGL
find_package(OpenGL REQUIRED)

target_link_libraries(BGECore PUBLIC 
    glfw 
    Threads::Threads
    imgui::imgui
    OpenGL::GL
)

# Platform-specific implementations
if(WIN32)
    target_sources(BGECore PRIVATE Platform/Windows/WindowWin32.cpp)
elseif(UNIX)
    target_sources(BGECore PRIVATE Platform/Linux/WindowLinux.cpp)
endif()
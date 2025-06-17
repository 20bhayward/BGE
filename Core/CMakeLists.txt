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
    
    # UI System - Framework
    UI/Framework/UISystem.h
    UI/Framework/UISystem.cpp
    UI/Framework/Panel.h
    UI/Framework/PanelManager.h
    UI/Framework/LayoutInfo.h
    
    # UI System - Docking
    UI/Docking/DockNode.h
    UI/Docking/DockNode.cpp
    UI/Docking/DockingSystem.h
    UI/Docking/DockingSystem.cpp
    UI/Docking/LayoutSerializer.h
    UI/Docking/LayoutSerializer.cpp
    UI/Docking/LayoutData.h
    UI/Docking/LayoutData.cpp
    
    # UI System - Panels
    UI/Panels/AssetBrowserPanel.h
    UI/Panels/AssetBrowserPanel.cpp
    UI/Panels/ProjectSettingsPanel.h
    UI/Panels/ProjectSettingsPanel.cpp
    UI/Panels/SceneViewPanel.h
    UI/Panels/SceneViewPanel.cpp
    UI/Panels/GameViewportPanel.h
    UI/Panels/GameViewportPanel.cpp
    UI/Panels/InspectorPanel.h
    UI/Panels/InspectorPanel.cpp
    UI/Panels/HierarchyPanel.h
    UI/Panels/HierarchyPanel.cpp
    UI/Panels/MaterialPalettePanel.h
    UI/Panels/MaterialPalettePanel.cpp
    UI/Panels/MaterialEditorPanel.h
    UI/Panels/MaterialEditorPanel.cpp
    UI/Panels/ConsolePanel.h
    UI/Panels/ConsolePanel.cpp
    
    # UI System - Icon Management
    UI/IconManager.h
    UI/IconManager.cpp
    
    # UI System - Layout
    UI/Layout/UnityLayoutManager.h
    UI/Layout/UnityLayoutManager.cpp
    
    # UI System - Legacy
    UI/Legacy/MaterialEditorUI.h
    UI/Legacy/MaterialEditorUI.cpp
    UI/Legacy/TabbedPanel.h
    UI/Legacy/TabbedPanel.cpp
    
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
    BGEAssetPipeline
)

# Platform-specific implementations
if(WIN32)
    target_sources(BGECore PRIVATE Platform/Windows/WindowWin32.cpp)
elseif(UNIX)
    target_sources(BGECore PRIVATE Platform/Linux/WindowLinux.cpp)
endif()
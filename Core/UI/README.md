# UI System Architecture

The UI system has been reorganized into a modular, well-structured architecture with clear separation of concerns.

## Folder Structure

```
Core/UI/
├── Framework/           # Core UI framework components
│   ├── Panel.h         # Base panel interface
│   ├── PanelManager.h  # Panel registry and management
│   ├── UISystem.cpp/.h # Main UI system coordinator
│   └── LayoutInfo.h    # Layout information structures
├── Docking/            # Docking system implementation
│   ├── DockNode.cpp/.h        # Individual dock nodes
│   ├── DockingSystem.cpp/.h   # Main docking logic
│   └── LayoutSerializer.cpp/.h # Layout persistence
├── Panels/             # Application-specific panels
│   ├── AssetBrowserPanel.cpp/.h
│   ├── ConsolePanel.cpp/.h
│   ├── GameViewportPanel.cpp/.h
│   ├── HierarchyPanel.cpp/.h
│   ├── InspectorPanel.cpp/.h
│   ├── MaterialEditorPanel.cpp/.h
│   ├── MaterialPalettePanel.cpp/.h
│   └── SceneViewPanel.cpp/.h
├── Layout/             # Layout management
│   └── UnityLayoutManager.cpp/.h
└── Legacy/             # Deprecated/transitional code
    ├── MaterialEditorUI.cpp/.h
    └── TabbedPanel.cpp/.h
```

## Architecture Principles

### 1. Separation of Concerns
- **Framework**: Core UI abstractions and system coordination
- **Docking**: Self-contained docking system with layout management
- **Panels**: Application-specific UI components
- **Layout**: High-level layout management utilities
- **Legacy**: Transitional code to be phased out

### 2. Clear Dependencies
- Panels depend on Framework (Panel base class)
- Docking depends on Framework (Panel interface)
- UISystem coordinates all subsystems
- Legacy components are isolated

### 3. Extensibility
- New panels can be easily added to Panels/ folder
- Framework provides stable base classes
- Docking system is self-contained and reusable

## Key Components

### UISystem (Framework/)
- Main coordinator for all UI subsystems
- Manages ImGui initialization and lifecycle
- Provides centralized layout menu rendering
- Integrates docking system and panel manager

### DockingSystem (Docking/)
- Custom docking implementation for panel management
- Handles drag & drop, resizing, and layout persistence
- Supports custom layout save/load functionality
- Independent of specific panel implementations

### Panel Base Class (Framework/)
- Abstract base for all UI panels
- Provides common functionality (visibility, rendering lifecycle)
- ImGui integration with proper window management

### Panel Implementations (Panels/)
- Application-specific panels inherit from Panel base
- Self-contained with clear responsibilities
- Follow consistent patterns for initialization and rendering

## Migration Path

The Legacy/ folder contains components that are being transitioned:
- **MaterialEditorUI**: High-level editor coordinator (to be simplified)
- **TabbedPanel**: Old tabbing system (replaced by docking)

These will be gradually refactored or removed as the new architecture matures.

## Include Patterns

When including UI components, use the full path:
```cpp
#include "UI/Framework/UISystem.h"
#include "UI/Panels/HierarchyPanel.h"
#include "UI/Docking/DockingSystem.h"
```

This makes dependencies explicit and improves build organization.
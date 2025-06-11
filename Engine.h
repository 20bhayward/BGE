#pragma once

#include "Window.h"
#include "EventHandler.h"
#include "Grid.h"
#include "Renderer.h"
#include "MaterialRegistry.h"
#include "UIManager.h"

#include <memory> // For std::unique_ptr

// Forward declarations (though includes are already present,
// it's good practice in more complex scenarios to minimize include dependencies in headers)
class Window;
class Grid;
class Renderer;
class MaterialRegistry;
class UIManager;

class Engine : public EventHandler {
public:
    Engine();
    ~Engine() override; // It's good practice to mark override for virtual destructors

    void run();

    // EventHandler overrides
    void onClose() override;
    void onKeyPressed(int key) override;
    void onKeyReleased(int key) override;
    void onMouseButtonPressed(int button, int x, int y) override;
    void onMouseButtonReleased(int button, int x, int y) override;
    void onMouseMove(int x, int y) override;

private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Grid> m_grid;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<MaterialRegistry> m_materialRegistry;
    std::unique_ptr<UIManager> m_uiManager;

    bool m_isRunning;
};

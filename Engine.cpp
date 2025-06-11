#include "Engine.h"
#include "Window.h"
#include "Grid.h"
#include "Renderer.h"
#include "MaterialRegistry.h"
#include "UIManager.h"

// Include iostream for temporary debug output, if needed
#include <iostream>

Engine::Engine() : m_isRunning(true) {
    // Initialize members using std::make_unique
    // For now, we'll assume default constructors or pass some sensible defaults.
    // These will likely need to be adjusted later based on actual class definitions.

    m_window = std::make_unique<Window>();
    // TODO: Window creation might need parameters like width, height, title
    // Example: m_window->create(800, 600, "Sand Simulation");

    m_materialRegistry = std::make_unique<MaterialRegistry>();

    // Assuming Grid takes width and height, and now MaterialRegistry
    m_grid = std::make_unique<Grid>(100, 100, *m_materialRegistry); // Example dimensions, added m_materialRegistry

    // Assuming Renderer takes width and height, or a reference to the window
    // If Renderer needs window dimensions, we might need to get them from m_window
    // For now, let's assume it can be default constructed or takes some defaults.
    // This might need to be: m_renderer = std::make_unique<Renderer>(*m_window);
    // Or, if Window::create needs to be called first:
    // m_window->create(800, 600, "Title");
    // m_renderer = std::make_unique<Renderer>(800, 600);
    m_renderer = std::make_unique<Renderer>(*m_materialRegistry); // Pass MaterialRegistry

    m_uiManager = std::make_unique<UIManager>();

    // It's good practice to check if initializations were successful,
    // though std::make_unique will throw on allocation failure.
    // For classes that have their own internal initialization steps that can fail,
    // you might add checks here.

    if (m_window) {
        m_window->setEventHandler(this); // Set the Engine itself as the event handler for the Window
    }

    std::cout << "Engine initialized." << std::endl; // Temporary debug output
}

Engine::~Engine() {
    // The std::unique_ptr members will automatically clean up their managed objects.
    // If there's any other explicit cleanup needed before that, it would go here.
    std::cout << "Engine destroyed." << std::endl; // Temporary debug output
}

#include <chrono> // For std::chrono

// Placeholder for a potential processInput method if it were part of Engine
// For now, event handling is done through the EventHandler methods.
// We might later add a specific processInput phase if needed.
// void Engine::processInput() {
// std::cout << "Processing input..." << std::endl;
// }

void Engine::run() {
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_isRunning) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;

        // processInput(); // Call a general input processing method if you have one

        // Window event polling should happen here.
        // This is crucial for the window to respond and for events to be dispatched.
        // Assuming Window class has a pollEvents method that would call our EventHandler methods.
        if (m_window) {
            m_window->pollEvents();
        }

        // If m_isRunning became false from an event like onClose, break early.
        if (!m_isRunning) {
            break;
        }

        if (m_uiManager) {
            m_uiManager->update(deltaTime);
        }

        if (m_grid) {
            m_grid->update(deltaTime);
            m_grid->swapBuffers(); // Add this line
        }

        if (m_renderer && m_grid && m_window) { // Ensure window is also valid
            m_renderer->render(*m_grid, *m_window); // Pass Grid and Window
        }

        // If UIManager has its own rendering pass (e.g., for ImGui)
        if (m_uiManager) {
            // m_uiManager->render(); // Assuming UIManager has a render method
        }

        if (m_window) {
            m_window->display(); // Swaps buffers and displays the rendered frame
        }
    }
}

void Engine::onClose() {
    m_isRunning = false;
    std::cout << "Engine: onClose event received. Shutting down." << std::endl;
}

void Engine::onKeyPressed(int key) {
    std::cout << "Engine: Key pressed: " << key << std::endl;
    // TODO: Handle key press (e.g., pass to UIManager, select material)
    // if (m_uiManager) m_uiManager->processKeyPressed(key);
}

void Engine::onKeyReleased(int key) {
    std::cout << "Engine: Key released: " << key << std::endl;
    // TODO: Handle key release
    // if (m_uiManager) m_uiManager->processKeyReleased(key);
}

void Engine::onMouseButtonPressed(int button, int x, int y) {
    std::cout << "Engine: Mouse button " << button << " pressed at (" << x << ", " << y << ")" << std::endl;
    // TODO: Implement sand placement:
    // if (m_grid) m_grid->placeSand(x, y);
    // Consider mapping window coordinates to grid coordinates if necessary.
}

void Engine::onMouseButtonReleased(int button, int x, int y) {
    std::cout << "Engine: Mouse button " << button << " released at (" << x << ", " << y << ")" << std::endl;
    // TODO: Handle mouse button release
    // if (m_uiManager) m_uiManager->processMouseButtonReleased(button, x, y);
}

void Engine::onMouseMove(int x, int y) {
    // This event can be very frequent, so logging it might be too verbose.
    // std::cout << "Engine: Mouse moved to (" << x << ", " << y << ")" << std::endl;
    // TODO: Handle mouse move (e.g., for UI interactions, dragging)
    // if (m_uiManager) m_uiManager->processMouseMove(x, y);
}

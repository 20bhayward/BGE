#include "Window.h"
#include "EventHandler.h" // Required for EventHandler interaction
#include <iostream> // For debug messages

Window::Window() : m_window_handle(nullptr), m_eventHandler(nullptr) {
    // In a real application, this would initialize the windowing system (e.g., GLFW, SDL)
    // For now, we'll just simulate a window being "created" conceptually.
    std::cout << "Window: Constructor called." << std::endl;
    // create(800, 600, "Default Window"); // Example creation
}

Window::~Window() {
    std::cout << "Window: Destructor called." << std::endl;
    // In a real application, this would destroy the window and terminate the windowing system.
    close();
}

void Window::create(unsigned int width, unsigned int height, const char* title) {
    std::cout << "Window: Creating window with size " << width << "x" << height << " and title \"" << title << "\"" << std::endl;
    // Placeholder for actual window creation logic
    // m_window_handle = ... (e.g., glfwCreateWindow)
    // For now, we'll assume it's always successful.
    if (m_window_handle) { // If it was already created, destroy first
        // cleanup existing window
    }
    // Simulate creating a new window handle
    m_window_handle = reinterpret_cast<void*>(new char[1]); // Dummy handle
}

void Window::close() {
    std::cout << "Window: Closing window." << std::endl;
    if (m_window_handle) {
        delete[] reinterpret_cast<char*>(m_window_handle); // Clean up dummy handle
        m_window_handle = nullptr;
    }
}

bool Window::isOpen() const {
    // In a real app, this would check the windowing system's state
    // For testing, let's assume it's open until close() is called or onClose event.
    return m_window_handle != nullptr; // Simplistic check based on our dummy handle
}

void Window::pollEvents() {
    // This is a critical part of a game loop.
    // It should process all pending window events (input, resize, close, etc.)
    // and dispatch them to the registered event handler.
    // std::cout << "Window: Polling events..." << std::endl; // Can be too verbose

    // SIMULATION: For now, to test the main loop termination via Engine::onClose(),
    // we will directly call onClose() on the handler.
    // In a real scenario, this would be driven by actual window events (e.g., user clicking X).
    if (m_eventHandler) {
        // This is a HACK to make the loop terminate for the current test setup.
        // In a real application, you would check for a specific close event from the OS/library
        // (e.g., if (glfwWindowShouldClose(m_window_handle)) m_eventHandler->onClose(); )
        // For now, let's call it once to ensure the engine loop can terminate.
        // To prevent immediate exit, we can add a counter or a flag.
        static int poll_count = 0;
        if (poll_count < 500) { // Let it run for a few "frames"
             // Simulate other events (optional)
            if (poll_count % 100 == 0 && poll_count > 0) { // Simulate a key press every 100 polls
                 // m_eventHandler->onKeyPressed('A');
            }
             if (poll_count % 150 == 0 && poll_count > 0) { // Simulate a mouse click
                 // m_eventHandler->onMouseButtonPressed(0, 10, 10);
            }
            poll_count++;
        } else if (poll_count == 500) { // Trigger close after some time
            std::cout << "Window: Simulating close event for termination test." << std::endl;
            m_eventHandler->onClose();
            poll_count++; // Prevent re-triggering immediately
        }
    }
}

void Window::display() {
    // In a real application, this would swap the front and back buffers
    // (e.g., glfwSwapBuffers(m_window_handle))
    // std::cout << "Window: Displaying/Swapping buffers." << std::endl; // Can be too verbose
}

void Window::setEventHandler(EventHandler* handler) {
    std::cout << "Window: Setting event handler." << std::endl;
    m_eventHandler = handler;
}

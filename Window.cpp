#include "Window.h"
#include "EventHandler.h"

// Constructor
Window::Window() : m_eventHandler(nullptr) {
    // Initialization, if any, other than m_eventHandler
}

// Destructor
Window::~Window() {
    // Cleanup, if any
    // sf::RenderWindow handles its own resources
}

// Create window
void Window::create(unsigned int width, unsigned int height, const char* title) {
    m_sfmlWindow.create(sf::VideoMode(width, height), title);
}

// Close window
void Window::close() {
    m_sfmlWindow.close();
}

// Check if window is open
bool Window::isOpen() const {
    return m_sfmlWindow.isOpen();
}

// Poll events
void Window::pollEvents() {
    sf::Event event;
    while (m_sfmlWindow.pollEvent(event)) {
        if (m_eventHandler) { // Ensure eventHandler is set
            if (event.type == sf::Event::Closed) {
                m_eventHandler->onClose();
            }
            // Example for MouseButtonPressed, adapt as needed for other events
            if (event.type == sf::Event::MouseButtonPressed) {
                // Convert SFML mouse button to your engine's format if necessary
                // For now, assuming direct pass-through or that your EventHandler
                // can handle sf::Mouse::Button directly or you'll adapt it.
                m_eventHandler->onMouseButtonPressed(event.mouseButton.button, event.mouseButton.x, event.mouseButton.y);
            }
            // Add other event types and handler calls as needed:
            // sf::Event::MouseMoved, sf::Event::KeyPressed, etc.
        }
    }
}

// Display window contents
void Window::display() {
    m_sfmlWindow.display();
}

// Set event handler
void Window::setEventHandler(EventHandler* handler) {
    m_eventHandler = handler;
}

// Draw drawable object
void Window::draw(const sf::Drawable& drawable) {
    m_sfmlWindow.draw(drawable);
}

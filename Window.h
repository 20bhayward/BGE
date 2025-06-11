#pragma once

#include <SFML/Graphics.hpp> // Added this line

class EventHandler; // Forward declaration

class Window {
public:
    Window();
    ~Window();

    void create(unsigned int width, unsigned int height, const char* title);
    void close();
    bool isOpen() const;
    void pollEvents(); // This will now use the m_eventHandler
    void display();
    void setEventHandler(EventHandler* handler);
    void draw(const sf::Drawable& drawable); // Added this line

private:
    // Placeholder for window implementation details
    sf::RenderWindow m_sfmlWindow; // Replaced void* m_window_handle
    EventHandler* m_eventHandler; // Pointer to the event handler (the Engine)
};

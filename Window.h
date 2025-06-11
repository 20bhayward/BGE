#pragma once

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

private:
    // Placeholder for window implementation details
    void* m_window_handle;
    EventHandler* m_eventHandler; // Pointer to the event handler (the Engine)
};

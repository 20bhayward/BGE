#pragma once

class EventHandler {
public:
    virtual ~EventHandler() = default;

    virtual void onClose() = 0;
    virtual void onKeyPressed(int key) = 0;
    virtual void onKeyReleased(int key) = 0;
    virtual void onMouseButtonPressed(int button, int x, int y) = 0;
    virtual void onMouseButtonReleased(int button, int x, int y) = 0;
    virtual void onMouseMove(int x, int y) = 0;
};

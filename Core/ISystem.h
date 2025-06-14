#pragma once

#include <string> // Required for std::string if GetName returns it, or for potential future use.

namespace BGE
{

class ISystem
{
public:
    virtual ~ISystem() = default; // Important for proper cleanup of derived classes through base pointers.

    // Updates the system's logic.
    // deltaTime: The time elapsed since the last frame, in seconds.
    virtual void Update(float deltaTime) = 0;

    // Returns the name of the system. Useful for debugging or identification.
    // Consider returning const char* for simplicity if std::string isn't strictly needed yet,
    // but std::string is more flexible.
    virtual const char* GetName() const = 0; // Added const qualifier as GetName should not modify the system.
};

} // namespace BGE

#include "../Window.h"
#include "../Platform.h"

#ifdef BGE_PLATFORM_WINDOWS

#include <GLFW/glfw3.h>
#include <iostream>
#include "../../Input/InputManager.h"

namespace BGE {

// Windows-specific window implementation
class WindowImpl {
public:
    GLFWwindow* glfwWindow = nullptr;
    bool shouldClose = false;
    InputManager* inputManager = nullptr;
    
    static void ErrorCallback(int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    }
    
    static void WindowCloseCallback(GLFWwindow* window) {
        WindowImpl* impl = static_cast<WindowImpl*>(glfwGetWindowUserPointer(window));
        if (impl) {
            impl->shouldClose = true;
        }
    }
    
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        (void)scancode; (void)mods;
        WindowImpl* impl = static_cast<WindowImpl*>(glfwGetWindowUserPointer(window));
        if (impl && impl->inputManager) {
            if (action == GLFW_PRESS) {
                impl->inputManager->OnKeyPressed(key);
            } else if (action == GLFW_RELEASE) {
                impl->inputManager->OnKeyReleased(key);
            }
        }
    }
    
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        (void)mods;
        WindowImpl* impl = static_cast<WindowImpl*>(glfwGetWindowUserPointer(window));
        if (impl && impl->inputManager) {
            if (action == GLFW_PRESS) {
                impl->inputManager->OnMousePressed(button);
            } else if (action == GLFW_RELEASE) {
                impl->inputManager->OnMouseReleased(button);
            }
        }
    }
    
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        WindowImpl* impl = static_cast<WindowImpl*>(glfwGetWindowUserPointer(window));
        if (impl && impl->inputManager) {
            impl->inputManager->OnMouseMoved(static_cast<float>(xpos), static_cast<float>(ypos));
        }
    }
    
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        (void)xoffset;
        WindowImpl* impl = static_cast<WindowImpl*>(glfwGetWindowUserPointer(window));
        if (impl && impl->inputManager) {
            impl->inputManager->OnMouseWheel(static_cast<float>(yoffset));
        }
    }
};

Window::Window() : m_impl(std::make_unique<WindowImpl>()) {
}

Window::~Window() {
    Shutdown();
}

bool Window::Initialize(const WindowConfig& config) {
    // Initialize GLFW
    glfwSetErrorCallback(WindowImpl::ErrorCallback);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Configure GLFW for OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    
    // Create window
    GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_impl->glfwWindow = glfwCreateWindow(
        config.width, config.height,
        config.title.c_str(),
        monitor, nullptr
    );
    
    if (!m_impl->glfwWindow) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // Set up callbacks
    glfwSetWindowUserPointer(m_impl->glfwWindow, m_impl.get());
    glfwSetWindowCloseCallback(m_impl->glfwWindow, WindowImpl::WindowCloseCallback);
    glfwSetKeyCallback(m_impl->glfwWindow, WindowImpl::KeyCallback);
    glfwSetMouseButtonCallback(m_impl->glfwWindow, WindowImpl::MouseButtonCallback);
    glfwSetCursorPosCallback(m_impl->glfwWindow, WindowImpl::CursorPosCallback);
    glfwSetScrollCallback(m_impl->glfwWindow, WindowImpl::ScrollCallback);
    
    // Make context current
    glfwMakeContextCurrent(m_impl->glfwWindow);
    
    // Enable vsync
    if (config.vsync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }
    
    return true;
}

void Window::Shutdown() {
    if (m_impl->glfwWindow) {
        glfwDestroyWindow(m_impl->glfwWindow);
        m_impl->glfwWindow = nullptr;
    }
    glfwTerminate();
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    if (m_impl->glfwWindow) {
        glfwSwapBuffers(m_impl->glfwWindow);
    }
}

bool Window::ShouldClose() const {
    return m_impl->shouldClose || (m_impl->glfwWindow && glfwWindowShouldClose(m_impl->glfwWindow));
}

void Window::SetShouldClose(bool shouldClose) {
    m_impl->shouldClose = shouldClose;
    if (m_impl->glfwWindow) {
        glfwSetWindowShouldClose(m_impl->glfwWindow, shouldClose ? GLFW_TRUE : GLFW_FALSE);
    }
}

void Window::GetSize(int& width, int& height) const {
    if (m_impl->glfwWindow) {
        glfwGetWindowSize(m_impl->glfwWindow, &width, &height);
    } else {
        width = height = 0;
    }
}

void Window::SetSize(int width, int height) {
    if (m_impl->glfwWindow) {
        glfwSetWindowSize(m_impl->glfwWindow, width, height);
    }
}

void Window::GetFramebufferSize(int& width, int& height) const {
    if (m_impl->glfwWindow) {
        glfwGetFramebufferSize(m_impl->glfwWindow, &width, &height);
    } else {
        width = height = 0;
    }
}

void Window::SetTitle(const std::string& title) {
    if (m_impl->glfwWindow) {
        glfwSetWindowTitle(m_impl->glfwWindow, title.c_str());
    }
}

void Window::SetInputManager(InputManager* inputManager) {
    m_impl->inputManager = inputManager;
}

void* Window::GetNativeHandle() const {
    return m_impl->glfwWindow;
}

} // namespace BGE

#endif // BGE_PLATFORM_WINDOWS
#include "UISystem.h"
#include "../Platform/Window.h"

// ImGui includes (we'll need to add these to the project)
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <iostream>

namespace BGE {

UISystem::UISystem() = default;

UISystem::~UISystem() {
    if (m_initialized) {
        Shutdown();
    }
}

bool UISystem::Initialize(Window* window) {
    if (m_initialized || !window) {
        return false;
    }
    
    m_window = window;
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // Set up ImGui ini file path for saving layouts
    static std::string iniPath = "imgui_layout.ini";
    io.IniFilename = iniPath.c_str();
    
    // Setup Dear ImGui style
    SetDarkTheme();
    
    // Setup Platform/Renderer backends
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->GetNativeHandle());
    if (!ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
        return false;
    }
    
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        ImGui_ImplGlfw_Shutdown();
        return false;
    }
    
    m_initialized = true;
    std::cout << "UI System initialized successfully" << std::endl;
    return true;
}

void UISystem::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    m_initialized = false;
    m_window = nullptr;
    
    std::cout << "UI System shutdown complete" << std::endl;
}

void UISystem::BeginFrame() {
    if (!m_initialized || !m_enabled) {
        return;
    }
    
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UISystem::EndFrame() {
    if (!m_initialized || !m_enabled) {
        return;
    }
    
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool UISystem::WantCaptureMouse() const {
    if (!m_initialized || !m_enabled) {
        return false;
    }
    return ImGui::GetIO().WantCaptureMouse;
}

bool UISystem::WantCaptureKeyboard() const {
    if (!m_initialized || !m_enabled) {
        return false;
    }
    return ImGui::GetIO().WantCaptureKeyboard;
}

void UISystem::SetDarkTheme() {
    ImGui::StyleColorsDark();
    
    // Custom dark theme tweaks
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;
    
    // Custom colors for material editor
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
}

void UISystem::SetLightTheme() {
    ImGui::StyleColorsLight();
}

void UISystem::SetCustomTheme() {
    SetDarkTheme(); // Default to dark theme for now
}

void UISystem::SetupStyle() {
    // Additional style setup if needed
}

// UI Helper functions
namespace UI {

bool ColoredButton(const char* label, float r, float g, float b, float a) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, a));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.2f, g * 1.2f, b * 1.2f, a));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, a));
    bool result = ImGui::Button(label);
    ImGui::PopStyleColor(3);
    return result;
}

void MaterialColor(const char* label, uint32_t color) {
    float r = ((color >> 0) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = ((color >> 16) & 0xFF) / 255.0f;
    float a = ((color >> 24) & 0xFF) / 255.0f;
    
    ImGui::ColorButton(label, ImVec4(r, g, b, a), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop);
}

bool SliderWithReset(const char* label, int* value, int min, int max, int defaultValue) {
    bool changed = ImGui::SliderInt(label, value, min, max);
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        *value = defaultValue;
        changed = true;
    }
    return changed;
}

void StatusText(const char* label, const char* text) {
    ImGui::Text("%s: %s", label, text);
}

void Separator() {
    ImGui::Separator();
}

void Spacing() {
    ImGui::Spacing();
}

} // namespace UI

void UISystem::BeginDockspace() {
    // Simulated Unity-style docking using child windows
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;
    
    // Create invisible fullscreen window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(displaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("##DockSpace", nullptr, flags);
    ImGui::PopStyleVar(3);
    
    // Calculate Unity-style layout areas
    float menuBarHeight = 20.0f;
    float leftPanelWidth = 250.0f;
    float rightPanelWidth = 300.0f;
    float topToolbarHeight = 35.0f;
    float bottomPanelHeight = 150.0f;
    
    float centerHeight = displaySize.y - menuBarHeight - topToolbarHeight - bottomPanelHeight;
    float centerStartY = menuBarHeight + topToolbarHeight;
    
    m_layoutInfo.leftArea = ImVec4(0, menuBarHeight, leftPanelWidth, centerHeight + topToolbarHeight);
    m_layoutInfo.topToolbarArea = ImVec4(leftPanelWidth, menuBarHeight,
                                        displaySize.x - leftPanelWidth - rightPanelWidth,
                                        topToolbarHeight);
    m_layoutInfo.centerArea = ImVec4(leftPanelWidth, centerStartY, 
                                    displaySize.x - leftPanelWidth - rightPanelWidth, 
                                    centerHeight);
    m_layoutInfo.rightArea = ImVec4(displaySize.x - rightPanelWidth, menuBarHeight, 
                                   rightPanelWidth, centerHeight + topToolbarHeight);
    m_layoutInfo.bottomArea = ImVec4(0, centerStartY + centerHeight,
                                    displaySize.x, bottomPanelHeight);
}

void UISystem::EndDockspace() {
    ImGui::End(); // End DockSpace window
}

bool UISystem::IsDockingEnabled() const {
    // Return true for simulated docking
    return true;
}

} // namespace BGE
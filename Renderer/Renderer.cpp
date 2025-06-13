#include "Renderer.h"
#include "Lighting/LightingSystem.h"
#include "../Core/Platform/Window.h"
#include "../Simulation/SimulationWorld.h"
#include <iostream>
#include <chrono>

#ifdef BGE_OPENGL_SUPPORT
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

namespace BGE {

Renderer::Renderer() = default;

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize(Window* window) {
    if (m_initialized) {
        return true;
    }
    
    m_window = window;
    
    std::cout << "Initializing renderer..." << std::endl;
    
    try {
        InitializeGraphicsAPI();
        CreateRenderTargets();
        SetupDefaultPipeline();
        
        // Initialize lighting system
        m_lightingSystem = std::make_unique<LightingSystem>();
        if (!m_lightingSystem->Initialize(this)) {
            std::cerr << "Failed to initialize lighting system" << std::endl;
            return false;
        }
        
        m_initialized = true;
        std::cout << "Renderer initialized successfully!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Renderer initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void Renderer::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down renderer..." << std::endl;
    
    m_lightingSystem.reset();
    m_window = nullptr;
    m_initialized = false;
    
    std::cout << "Renderer shutdown complete." << std::endl;
}

void Renderer::BeginFrame() {
    auto startTime = std::chrono::high_resolution_clock::now();
    
#ifdef BGE_OPENGL_SUPPORT
    // Make sure OpenGL context is current!
    GLFWwindow* window = static_cast<GLFWwindow*>(m_window->GetNativeHandle());
    if (window) {
        glfwMakeContextCurrent(window);
    }
    
    // Clear to black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Set viewport to window size
    int width, height;
    m_window->GetFramebufferSize(width, height);
    glViewport(0, 0, width, height);
    
    // Debug: Confirm BeginFrame is being called
    static int beginFrameCounter = 0;
    if (++beginFrameCounter % 120 == 0) {
        std::cout << "BeginFrame called " << beginFrameCounter << " times, viewport: " << width << "x" << height << std::endl;
    }
#endif
    
    // Calculate frame time from previous frame
    static auto lastTime = startTime;
    auto deltaTime = std::chrono::duration<float>(startTime - lastTime).count();
    lastTime = startTime;
    m_lastFrameTime = deltaTime;
}

void Renderer::EndFrame() {
    // Don't swap buffers here - let the Engine handle it to avoid double swapping!
    
    // Update statistics
    ++m_frameCount;
    
    if (m_frameCount % 60 == 0) {
        std::cout << "Rendered " << m_frameCount << " frames, last frame time: " 
                  << (m_lastFrameTime * 1000.0f) << "ms" << std::endl;
    }
}

void Renderer::RenderWorld(SimulationWorld* world) {
    if (!world) return;
    
    // Get world pixel data
    const uint8_t* pixelData = world->GetPixelData();
    uint32_t width = world->GetWidth();
    uint32_t height = world->GetHeight();
    
    // Update lighting if enabled
    if (m_lightingSystem) {
        m_lightingSystem->Update(world);
    }
    
#ifdef BGE_OPENGL_SUPPORT
    // Upload simulation data to GPU texture and render
    if (pixelData) {
        UpdateSimulationTexture(pixelData, width, height);
        RenderFullscreenQuad();
        
        // Debug: Print rendering info occasionally
        static int renderCounter = 0;
        if (++renderCounter % 60 == 0) {
            std::cout << "Rendering frame " << renderCounter << " - texture updated and quad rendered" << std::endl;
        }
    } else {
        static bool pixelWarningPrinted = false;
        if (!pixelWarningPrinted) {
            std::cout << "WARNING: No pixel data received for rendering!" << std::endl;
            pixelWarningPrinted = true;
        }
    }
#endif
    
    // Debug output occasionally
    static int logCounter = 0;
    if (++logCounter % 120 == 0) { // Every 2 seconds
        uint32_t activeCells = world->GetActiveCells();
        std::cout << "World render: " << width << "x" << height 
                  << ", active cells: " << activeCells << std::endl;
    }
}

void Renderer::InitializeGraphicsAPI() {
    std::cout << "Initializing OpenGL..." << std::endl;
    
#ifdef BGE_OPENGL_SUPPORT
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
    
    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;
    
    // Enable basic OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
#endif
}

void Renderer::CreateRenderTargets() {
    std::cout << "Creating render targets..." << std::endl;
    // Create framebuffers, textures, etc.
}

void Renderer::SetupDefaultPipeline() {
    std::cout << "Setting up rendering pipeline..." << std::endl;
    
#ifdef BGE_OPENGL_SUPPORT
    // Simple vertex shader for fullscreen quad
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        layout (location = 1) in vec2 texCoord;
        
        out vec2 TexCoord;
        
        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
            TexCoord = texCoord;
        }
    )";
    
    // Fragment shader for simulation rendering
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        
        uniform sampler2D simulationTexture;
        
        void main() {
            vec4 color = texture(simulationTexture, TexCoord);
            FragColor = vec4(color.rgb, 1.0);
        }
    )";
    
    // Compile shaders
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    // Create shader program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    
    // Clean up individual shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Create fullscreen quad
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
    };
    
    uint32_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    uint32_t EBO;
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(m_quadVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
#endif
}

void Renderer::CreateSimulationTexture(uint32_t width, uint32_t height) {
#ifdef BGE_OPENGL_SUPPORT
    glGenTextures(1, &m_simulationTexture);
    glBindTexture(GL_TEXTURE_2D, m_simulationTexture);
    
    // Create empty texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

void Renderer::UpdateSimulationTexture(const uint8_t* pixelData, uint32_t width, uint32_t height) {
#ifdef BGE_OPENGL_SUPPORT
    if (m_simulationTexture == 0) {
        CreateSimulationTexture(width, height);
    }
    
    glBindTexture(GL_TEXTURE_2D, m_simulationTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

void Renderer::RenderFullscreenQuad() {
#ifdef BGE_OPENGL_SUPPORT
    // Check for OpenGL errors before rendering
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error before quad render: " << error << std::endl;
    }
    
    glUseProgram(m_shaderProgram);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_simulationTexture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "simulationTexture"), 0);
    
    glBindVertexArray(m_quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Check for OpenGL errors after rendering
    error = glGetError();
    if (error != GL_NO_ERROR) {
        static bool errorPrinted = false;
        if (!errorPrinted) {
            std::cout << "OpenGL error after quad render: " << error << std::endl;
            errorPrinted = true;
        }
    }
    
    // Debug: Verify we actually drew something
    static int quadRenderCount = 0;
    if (++quadRenderCount % 60 == 0) {
        std::cout << "Quad rendered " << quadRenderCount << " times, shader: " << m_shaderProgram << ", texture: " << m_simulationTexture << ", VAO: " << m_quadVAO << std::endl;
    }
#endif
}

} // namespace BGE
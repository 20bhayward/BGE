#include "Renderer.h"
#include "PixelCamera.h"
#include "ParticleSystem.h" // Keep for ParticleSystem::Render interface potentially
#include "../Core/Logger.h"
#include "../Core/ServiceLocator.h" // For ServiceLocator
#include "../Simulation/SimulationWorld.h"
#include <GLFW/glfw3.h>

namespace BGE {

Renderer::Renderer() {
    // Constructor
}

Renderer::~Renderer() {
    // Destructor
}

bool Renderer::Initialize(Window* window) {
    m_window = window;
    if (!m_window) {
        BGE_LOG_ERROR("Renderer", "Window pointer is null during initialization.");
        return false;
    }

    m_pixelCamera = std::make_unique<PixelCamera>();
    if (m_pixelCamera) {
        // Example: Set projection based on window size if available
        if (m_window) {
            int width, height;
            m_window->GetSize(width, height);
            m_pixelCamera->SetProjection(static_cast<float>(width), static_cast<float>(height));
             BGE_LOG_INFO("Renderer", "PixelCamera initialized and projection set.");
        } else {
             BGE_LOG_INFO("Renderer", "PixelCamera initialized (window not available for initial projection).");
        }
    } else {
        BGE_LOG_ERROR("Renderer", "Failed to initialize PixelCamera.");
    }

    BGE_LOG_INFO("Renderer", "Renderer initialized successfully.");
    return true;
}

void Renderer::Shutdown() {
    BGE_LOG_INFO("Renderer", "Renderer shutdown.");
    m_pixelCamera.reset();
}

void Renderer::BeginFrame() {
    BGE_LOG_TRACE("Renderer", "BeginFrame() - Setting up OpenGL state");
    
    // Clear screen with a dark gray background
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    BGE_LOG_TRACE("Renderer", "Screen cleared with gray background");
    
    // Set OpenGL viewport to simulation rendering area
    // Note: OpenGL Y=0 is at bottom, but our viewport Y coordinates are from top
    int windowWidth, windowHeight;
    if (m_window) {
        m_window->GetSize(windowWidth, windowHeight);
        // Convert from top-left origin to bottom-left origin for OpenGL
        int openglY = windowHeight - m_simViewportY - m_simViewportHeight;
        glViewport(m_simViewportX, openglY, m_simViewportWidth, m_simViewportHeight);
        BGE_LOG_TRACE("Renderer", "OpenGL viewport set to (" + std::to_string(m_simViewportX) + "," + 
                      std::to_string(openglY) + ") size " + std::to_string(m_simViewportWidth) + 
                      "x" + std::to_string(m_simViewportHeight) + " (converted from window coords (" +
                      std::to_string(m_simViewportX) + "," + std::to_string(m_simViewportY) + "))");
    } else {
        glViewport(m_simViewportX, m_simViewportY, m_simViewportWidth, m_simViewportHeight);
        BGE_LOG_TRACE("Renderer", "OpenGL viewport set to (" + std::to_string(m_simViewportX) + "," + 
                      std::to_string(m_simViewportY) + ") size " + std::to_string(m_simViewportWidth) + 
                      "x" + std::to_string(m_simViewportHeight) + " (no window available for Y conversion)");
    }
    
    // Set up pixel-perfect orthographic projection for material rendering
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Use simulation world coordinates (512x512) with Y=0 at bottom (standard OpenGL)
    glOrtho(0, 512, 0, 512, -1, 1); // Bottom-left origin, standard OpenGL orientation
    BGE_LOG_TRACE("Renderer", "Projection matrix set to orthographic (0,0)-(512,512)");
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    BGE_LOG_TRACE("Renderer", "Modelview matrix reset to identity");
    
    // Enable alpha blending for transparent materials
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    BGE_LOG_TRACE("Renderer", "Alpha blending enabled");
    
    // Disable depth testing for 2D rendering
    glDisable(GL_DEPTH_TEST);
    BGE_LOG_TRACE("Renderer", "Depth testing disabled");
    
    // Make points larger so they're actually visible
    glPointSize(2.0f);
    BGE_LOG_TRACE("Renderer", "Point size set to 2.0f");
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        BGE_LOG_ERROR("Renderer", "OpenGL error in BeginFrame(): " + std::to_string(error));
    } else {
        BGE_LOG_TRACE("Renderer", "BeginFrame() completed successfully - no OpenGL errors");
    }
}

void Renderer::EndFrame() {
    BGE_LOG_TRACE("Renderer", "EndFrame() - Finalizing frame rendering");
    
    // Check for OpenGL errors before finishing
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        BGE_LOG_ERROR("Renderer", "OpenGL error before EndFrame(): " + std::to_string(error));
    }
    
    // Note: Buffer swapping is handled by the Window class
    BGE_LOG_TRACE("Renderer", "EndFrame() completed (buffer swap handled by Window)");
}

void Renderer::SetSimulationViewport(int x, int y, int width, int height) {
    m_simViewportX = x;
    m_simViewportY = y;
    m_simViewportWidth = width;
    m_simViewportHeight = height;
    
    BGE_LOG_INFO("Renderer", "Simulation viewport updated to: (" + std::to_string(x) + "," + 
                 std::to_string(y) + ") size " + std::to_string(width) + "x" + std::to_string(height));
}

void Renderer::RenderWorld(class SimulationWorld* world) {
    if (!world) return;
    
    // Get the pre-rendered pixel data from the simulation world
    const uint8_t* pixelData = world->GetPixelData();
    uint32_t width = world->GetWidth();
    uint32_t height = world->GetHeight();
    
    if (!pixelData) {
        BGE_LOG_ERROR("Renderer", "No pixel data available from SimulationWorld");
        return;
    }
    
    // Debug: Log world dimensions and check for non-empty pixels
    static int debugCounter = 0;
    if (debugCounter++ % 60 == 0) { // Log once per second at 60 FPS
        int nonEmptyPixels = 0;
        for (uint32_t i = 0; i < width * height * 4; i += 4) {
            if (pixelData[i + 3] > 0) { // Check alpha channel
                nonEmptyPixels++;
            }
        }
        BGE_LOG_INFO("Renderer", "World size: " + std::to_string(width) + "x" + std::to_string(height) + 
                     ", Non-empty pixels: " + std::to_string(nonEmptyPixels));
    }
    
    BGE_LOG_TRACE("Renderer", "Starting OpenGL point rendering");
    
    // For now, use a simple pixel-by-pixel OpenGL approach
    // This should be optimized to use textures in the future
    glBegin(GL_POINTS);
    
    int pixelsDrawn = 0;
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            // Get RGBA values from pixel buffer
            uint32_t pixelIndex = (y * width + x) * 4;
            uint8_t r = pixelData[pixelIndex + 0];
            uint8_t g = pixelData[pixelIndex + 1];
            uint8_t b = pixelData[pixelIndex + 2];
            uint8_t a = pixelData[pixelIndex + 3];
            
            // Skip transparent/empty pixels
            if (a == 0) continue;
            
            // Set color and draw pixel
            glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
            glVertex2i(x, y);
            pixelsDrawn++;
            
            // Debug: Log first few pixels
            if (pixelsDrawn <= 5) {
                BGE_LOG_TRACE("Renderer", "Drawing pixel #" + std::to_string(pixelsDrawn) + 
                             " at (" + std::to_string(x) + "," + std::to_string(y) + ") " +
                             "RGBA(" + std::to_string(r) + "," + std::to_string(g) + "," + 
                             std::to_string(b) + "," + std::to_string(a) + ")");
            }
        }
    }
    
    glEnd();
    
    BGE_LOG_TRACE("Renderer", "OpenGL rendering completed - drew " + std::to_string(pixelsDrawn) + " pixels");
    
    // Check for OpenGL errors after rendering
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        BGE_LOG_ERROR("Renderer", "OpenGL error after RenderWorld(): " + std::to_string(error));
    }
}

// Placeholder implementation for drawing a single pixel
void Renderer::DrawPrimitivePixel(int x, int y, const Vector3& color) {
    (void)x; (void)y; (void)color; // Suppress unused parameter warnings
    // This is a placeholder. Actual implementation depends on the graphics API.
    // Example: If using an OpenGL immediate mode style (highly simplified):
    // glColor3f(color.x, color.y, color.z);
    // glBegin(GL_POINTS);
    // glVertex2i(x, y);
    // glEnd();
    // BGE_LOG_VERY_VERBOSE("Renderer", "DrawPrimitivePixel at (" + std::to_string(x) + "," + std::to_string(y) + ") with color (" + std::to_string(color.x) + ", " + std::to_string(color.y) + ", " + std::to_string(color.z) + ")");
}

void Renderer::RenderParticles() {
    auto particleSystem = ServiceLocator::Instance().GetService<ParticleSystem>();
    if (particleSystem) {
        particleSystem->Render(*this); // Pass *this (the Renderer instance)
    }
}

// Asset management functions for texture handling
uint32_t Renderer::CreateTexture(int width, int height, int channels, const void* data) {
    (void)width; (void)height; (void)channels; (void)data; // Suppress unused parameter warnings
    uint32_t textureId = m_nextTextureId++;
    // BGE_LOG_INFO("Renderer", "CreateTexture called: ID " + std::to_string(textureId) + ", Size " + std::to_string(width) + "x" + std::to_string(height) + ", Channels " + std::to_string(channels));
    // In a real renderer:
    // - glGenTextures(1, &textureId);
    // - glBindTexture(GL_TEXTURE_2D, textureId);
    // - glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    // - Set texture parameters (filtering, wrapping)
    return textureId; // Return a dummy ID
}

void Renderer::UpdateTexture(uint32_t textureId, int width, int height, int channels, const void* data) {
    (void)textureId; (void)width; (void)height; (void)channels; (void)data; // Suppress unused parameter warnings
    // BGE_LOG_INFO("Renderer", "UpdateTexture called: ID " + std::to_string(textureId) + ", New Size " + std::to_string(width) + "x" + std::to_string(height) + ", Channels " + std::to_string(channels));
    // In a real renderer:
    // - glBindTexture(GL_TEXTURE_2D, textureId);
    // - glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
}

void Renderer::DeleteTexture(uint32_t textureId) {
    (void)textureId; // Suppress unused parameter warning
    // BGE_LOG_INFO("Renderer", "DeleteTexture called: ID " + std::to_string(textureId));
    // In a real renderer:
    // - glDeleteTextures(1, &textureId);
}

} // namespace BGE
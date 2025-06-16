#include "Renderer.h"
#include "PixelCamera.h"
#include "ParticleSystem.h"
#include "PostProcessor.h"
#include "../Core/Logger.h"
#include "../Core/ServiceLocator.h" // For ServiceLocator
#include "../Core/Math/Vector2.h"
#include "../Simulation/SimulationWorld.h"
#include <GLFW/glfw3.h>

// Define OpenGL extension function pointer types
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint *framebuffers);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY *PFNGLGENRENDERBUFFERSPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY *PFNGLBINDRENDERBUFFERPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRY *PFNGLRENDERBUFFERSTORAGEPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum target);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *PFNGLDELETERENDERBUFFERSPROC) (GLsizei n, const GLuint *renderbuffers);

// OpenGL extension function pointers (will be loaded at runtime)
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;

// OpenGL constants for framebuffer operations
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT 0x8D00
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT 0x1902
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Function to load OpenGL extensions
bool LoadFramebufferExtensions() {
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)glfwGetProcAddress("glGenFramebuffers");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)glfwGetProcAddress("glBindFramebuffer");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)glfwGetProcAddress("glFramebufferTexture2D");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)glfwGetProcAddress("glGenRenderbuffers");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)glfwGetProcAddress("glBindRenderbuffer");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)glfwGetProcAddress("glRenderbufferStorage");
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)glfwGetProcAddress("glFramebufferRenderbuffer");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)glfwGetProcAddress("glCheckFramebufferStatus");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)glfwGetProcAddress("glDeleteFramebuffers");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)glfwGetProcAddress("glDeleteRenderbuffers");
    
    return (glGenFramebuffers && glBindFramebuffer && glFramebufferTexture2D && 
            glGenRenderbuffers && glBindRenderbuffer && glRenderbufferStorage &&
            glFramebufferRenderbuffer && glCheckFramebufferStatus && 
            glDeleteFramebuffers && glDeleteRenderbuffers);
}

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

    // Load OpenGL framebuffer extensions
    if (!LoadFramebufferExtensions()) {
        BGE_LOG_WARNING("Renderer", "Failed to load OpenGL framebuffer extensions. Render-to-texture will be disabled.");
    } else {
        BGE_LOG_INFO("Renderer", "OpenGL framebuffer extensions loaded successfully.");
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

    // Initialize PostProcessor with simulation world dimensions (512x512)
    // The PostProcessor works on the simulation pixel data, not the window size
    m_postProcessor = std::make_unique<PostProcessor>();
    if (m_postProcessor) {
        if (m_postProcessor->Initialize(512, 512)) {
            BGE_LOG_INFO("Renderer", "PostProcessor initialized for 512x512 simulation world");
        } else {
            BGE_LOG_ERROR("Renderer", "Failed to initialize PostProcessor");
        }
    }

    BGE_LOG_INFO("Renderer", "Renderer initialized successfully.");
    return true;
}

void Renderer::Shutdown() {
    BGE_LOG_INFO("Renderer", "Renderer shutdown.");
    
    // Cleanup framebuffer
    DestroyGameFramebuffer();
    
    if (m_postProcessor) {
        m_postProcessor->Shutdown();
        m_postProcessor.reset();
    }
    m_pixelCamera.reset();
}

void Renderer::BeginFrame() {
    BGE_LOG_TRACE("Renderer", "BeginFrame() - Setting up OpenGL state");
    
    // If we're not rendering to texture, set up the normal framebuffer
    if (!m_renderingToTexture) {
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
    } else {
        BGE_LOG_TRACE("Renderer", "Rendering to texture - viewport already set by BeginRenderToTexture()");
    }
    
    // Set up pixel-perfect orthographic projection for material rendering
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Apply screen shake offset if available
    Vector2 shakeOffset = {0.0f, 0.0f};
    if (m_postProcessor && m_postProcessor->IsEffectEnabled(PostProcessEffect::ScreenShake)) {
        shakeOffset = m_postProcessor->GetShakeOffset();
    }
    
    // Use simulation world coordinates (512x512) with Y=0 at bottom (standard OpenGL)
    // Apply screen shake by adjusting the orthographic bounds
    float left = 0.0f + shakeOffset.x;
    float right = 512.0f + shakeOffset.x;
    float bottom = 0.0f + shakeOffset.y;
    float top = 512.0f + shakeOffset.y;
    
    glOrtho(left, right, bottom, top, -1, 1);
    BGE_LOG_TRACE("Renderer", "Projection matrix set to orthographic with shake offset (" + 
                  std::to_string(shakeOffset.x) + "," + std::to_string(shakeOffset.y) + ")");
    
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
    
    // Disable point size since we'll use quads for tight pixel coverage
    // glPointSize(2.0f);
    BGE_LOG_TRACE("Renderer", "Prepared for quad-based pixel rendering");
    
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
    const uint8_t* originalPixelData = world->GetPixelData();
    uint32_t width = world->GetWidth();
    uint32_t height = world->GetHeight();
    
    if (!originalPixelData) {
        BGE_LOG_ERROR("Renderer", "No pixel data available from SimulationWorld");
        return;
    }
    
    // Create a working copy for post-processing
    std::vector<uint8_t> workingPixelData(originalPixelData, originalPixelData + (width * height * 4));
    uint8_t* pixelData = workingPixelData.data();
    
    // Apply post-processing effects
    if (m_postProcessor) {
        m_postProcessor->ProcessFrame(pixelData, width, height);
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
    
    BGE_LOG_TRACE("Renderer", "Starting OpenGL quad rendering for tight pixels");
    
    // Use GL_QUADS to ensure tight pixel coverage without gaps
    glBegin(GL_QUADS);
    
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
            
            // Set color for this pixel
            glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
            
            // Draw a tight 1x1 pixel quad (no gaps)
            float x_f = static_cast<float>(x);
            float y_f = static_cast<float>(y);
            
            glVertex2f(x_f, y_f);               // Bottom-left
            glVertex2f(x_f + 1.0f, y_f);       // Bottom-right
            glVertex2f(x_f + 1.0f, y_f + 1.0f); // Top-right
            glVertex2f(x_f, y_f + 1.0f);       // Top-left
            
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

bool Renderer::CreateGameFramebuffer(int width, int height) {
    // Check if framebuffer extensions are available
    if (!glGenFramebuffers) {
        BGE_LOG_ERROR("Renderer", "OpenGL framebuffer extensions not available");
        return false;
    }
    
    // Cleanup existing framebuffer if any
    DestroyGameFramebuffer();
    
    m_gameTextureWidth = width;
    m_gameTextureHeight = height;
    
    // Generate framebuffer
    glGenFramebuffers(1, &m_gameFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_gameFramebuffer);
    
    // Create color texture
    glGenTextures(1, &m_gameTextureId);
    glBindTexture(GL_TEXTURE_2D, m_gameTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Attach color texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gameTextureId, 0);
    
    // Create depth buffer (optional, but good practice)
    glGenRenderbuffers(1, &m_gameDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_gameDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_gameDepthBuffer);
    
    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        BGE_LOG_ERROR("Renderer", "Framebuffer not complete! Status: " + std::to_string(status));
        DestroyGameFramebuffer();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    // Restore default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    BGE_LOG_INFO("Renderer", "Game framebuffer created successfully: " + std::to_string(width) + "x" + std::to_string(height) + 
                 ", Texture ID: " + std::to_string(m_gameTextureId));
    return true;
}

void Renderer::DestroyGameFramebuffer() {
    if (m_gameDepthBuffer != 0 && glDeleteRenderbuffers) {
        glDeleteRenderbuffers(1, &m_gameDepthBuffer);
        m_gameDepthBuffer = 0;
    }
    
    if (m_gameTextureId != 0) {
        glDeleteTextures(1, &m_gameTextureId);
        m_gameTextureId = 0;
    }
    
    if (m_gameFramebuffer != 0 && glDeleteFramebuffers) {
        glDeleteFramebuffers(1, &m_gameFramebuffer);
        m_gameFramebuffer = 0;
    }
    
    m_renderingToTexture = false;
    BGE_LOG_INFO("Renderer", "Game framebuffer destroyed");
}

void Renderer::BeginRenderToTexture() {
    if (m_gameFramebuffer == 0 || !glBindFramebuffer) {
        BGE_LOG_ERROR("Renderer", "Cannot begin render to texture - framebuffer not created or extensions not available");
        return;
    }
    
    // Bind our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_gameFramebuffer);
    
    // Set viewport to texture size
    glViewport(0, 0, m_gameTextureWidth, m_gameTextureHeight);
    
    // Clear the texture
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    m_renderingToTexture = true;
    
    BGE_LOG_TRACE("Renderer", "Started rendering to texture (" + 
                  std::to_string(m_gameTextureWidth) + "x" + std::to_string(m_gameTextureHeight) + ")");
}

void Renderer::EndRenderToTexture() {
    if (!m_renderingToTexture || !glBindFramebuffer) {
        return;
    }
    
    // Restore default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Restore window viewport (will be set again in BeginFrame if needed)
    if (m_window) {
        int windowWidth, windowHeight;
        m_window->GetSize(windowWidth, windowHeight);
        glViewport(0, 0, windowWidth, windowHeight);
    }
    
    m_renderingToTexture = false;
    
    BGE_LOG_TRACE("Renderer", "Finished rendering to texture");
}

} // namespace BGE
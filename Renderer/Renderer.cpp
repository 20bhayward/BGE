#include "Renderer.h"
#include "PixelCamera.h"
#include "ParticleSystem.h" // Keep for ParticleSystem::Render interface potentially
#include "../Core/Logger.h"
#include "../Core/ServiceLocator.h" // For ServiceLocator
// Temporary include for SimulationWorld, if RenderWorld needs it.
// #include "../Simulation/SimulationWorld.h"

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
        // This assumes m_window is valid and GetWidth/GetHeight exist.
        // If GetWidth/GetHeight are not on BGE::Window, this call needs adjustment
        // or PixelCamera::SetProjection must be called from somewhere else (e.g. Engine or App on resize)
        if (m_window) {
            m_pixelCamera->SetProjection(static_cast<float>(m_window->GetWidth()), static_cast<float>(m_window->GetHeight()));
             BGE_LOG_INFO("Renderer", "PixelCamera initialized and projection set.");
        } else {
             BGE_LOG_INFO("Renderer", "PixelCamera initialized (window not available for initial projection).");
        }
    } else {
        BGE_LOG_ERROR("Renderer", "Failed to initialize PixelCamera.");
        // return false; // Optionally propagate failure
    }

    // ParticleSystem removed from direct Renderer ownership
    // m_particleSystem = std::make_unique<ParticleSystem>();
    // if (m_particleSystem) {
    //     if (m_particleSystem->Initialize()) { // Default pool size
    //         BGE_LOG_INFO("Renderer", "ParticleSystem initialized successfully.");
    //     } else {
    //         BGE_LOG_ERROR("Renderer", "Failed to initialize ParticleSystem component.");
    //         m_particleSystem.reset(); // Nullify if initialization failed
    //         // return false; // Optionally propagate failure
    //     }
    // } else {
    //     BGE_LOG_ERROR("Renderer", "Failed to create ParticleSystem instance.");
    //     // return false; // Optionally propagate failure
    // }

    BGE_LOG_INFO("Renderer", "Renderer initialized successfully.");
    return true;
}

void Renderer::Shutdown() {
    BGE_LOG_INFO("Renderer", "Renderer shutdown.");
    // ParticleSystem removed from direct Renderer ownership
    // if (m_particleSystem) m_particleSystem->Shutdown();
    // m_particleSystem.reset();
    m_pixelCamera.reset();
}

void Renderer::BeginFrame() {
    // Clear screen, setup matrices etc.
    // For now, this might be handled by existing GL calls if porting from an old system.
    // Example: glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame() {
    // Swap buffers, present frame.
    // This is often handled by the Window class or platform-specific code.
}

// Temporary placeholder
void Renderer::RenderWorld(class SimulationWorld* world) {
    // This function is a placeholder to match Engine.cpp's current calls.
    // Actual world rendering logic will be more complex.
    if (world) {
        // BGE_LOG_TRACE("Renderer", "RenderWorld called (placeholder).");
    }
}

// Implementations for particle methods and GetViewMatrix will be added in later steps
// UpdateParticles is removed as Engine will call ParticleSystem::Update directly
// void Renderer::UpdateParticles(float deltaTime) {
//     auto particleSystem = ServiceLocator::Instance().GetService<ParticleSystem>();
//     if (particleSystem) {
//         particleSystem->Update(deltaTime);
//     }
// }

// Placeholder implementation for drawing a single pixel
void Renderer::DrawPrimitivePixel(int x, int y, const Vector3& color) {
    // This is a placeholder. Actual implementation depends on the graphics API.
    // Example: If using an OpenGL immediate mode style (highly simplified):
    // glColor3f(color.x, color.y, color.z);
    // glBegin(GL_POINTS);
    // glVertex2i(x, y);
    // glEnd();
    // BGE_LOG_VERY_VERBOSE("Renderer", "DrawPrimitivePixel at (" + std::to_string(x) + "," + std::to_string(y) + ") with color (" + std::to_string(color.x) + ", " + std::to_string(color.y) + ", " + std::to_string(color.z) + ")");
}
// Or for quads:
// void Renderer::DrawPrimitiveQuad(const Vector2& position, const Vector2& size, const Vector3& color) {
//    BGE_LOG_VERY_VERBOSE("Renderer", "DrawPrimitiveQuad at (" + std::to_string(position.x) + ...);
// }

void Renderer::RenderParticles() {
    auto particleSystem = ServiceLocator::Instance().GetService<ParticleSystem>();
    if (particleSystem) {
        particleSystem->Render(*this); // Pass *this (the Renderer instance)
    }
}
// Matrix4 Renderer::GetViewMatrix() const {
//     if (m_pixelCamera) return m_pixelCamera->GetViewMatrix();
//     return Matrix4(); // Return identity if no camera
// }

} // namespace BGE

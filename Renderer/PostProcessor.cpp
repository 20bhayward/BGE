#include "PostProcessor.h"
#include "../Core/Logger.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace BGE {

PostProcessor::PostProcessor() {
    // Constructor
}

PostProcessor::~PostProcessor() {
    // Destructor
}

bool PostProcessor::Initialize(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // Initialize working buffers
    size_t bufferSize = screenWidth * screenHeight * 4; // RGBA
    m_bloomBuffer.resize(bufferSize);
    m_tempBuffer.resize(bufferSize);

    BGE_LOG_INFO("PostProcessor", "Initialized for " + std::to_string(screenWidth) + 
                 "x" + std::to_string(screenHeight) + " resolution");
    return true;
}

void PostProcessor::Shutdown() {
    m_bloomBuffer.clear();
    m_tempBuffer.clear();
    BGE_LOG_INFO("PostProcessor", "Post-processor shutdown complete");
}

void PostProcessor::ProcessFrame(uint8_t* pixelData, int width, int height) {
    if (!pixelData) {
        BGE_LOG_ERROR("PostProcessor", "ProcessFrame called with null pixel data");
        return;
    }
    
    if (width != m_screenWidth || height != m_screenHeight) {
        BGE_LOG_ERROR("PostProcessor", "ProcessFrame size mismatch: expected " + 
                      std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight) +
                      ", got " + std::to_string(width) + "x" + std::to_string(height));
        return;
    }

    static int frameCounter = 0;
    bool hasEffects = m_enabledEffects != 0;
    
    // Apply effects in order
    if (IsEffectEnabled(PostProcessEffect::Bloom)) {
        ApplyBloom(pixelData, width, height);
        if (frameCounter % 60 == 0) {
            BGE_LOG_DEBUG("PostProcessor", "Applying Bloom effect");
        }
    }

    if (IsEffectEnabled(PostProcessEffect::ColorGrading)) {
        ApplyColorGrading(pixelData, width, height);
        if (frameCounter % 60 == 0) {
            BGE_LOG_DEBUG("PostProcessor", "Applying Color Grading effect");
        }
    }

    if (IsEffectEnabled(PostProcessEffect::Scanlines)) {
        ApplyScanlines(pixelData, width, height);
        if (frameCounter % 60 == 0) {
            BGE_LOG_DEBUG("PostProcessor", "Applying Scanlines effect");
        }
    }

    if (IsEffectEnabled(PostProcessEffect::Pixelation)) {
        ApplyPixelation(pixelData, width, height);
        if (frameCounter % 60 == 0) {
            BGE_LOG_DEBUG("PostProcessor", "Applying Pixelation effect");
        }
    }
    
    if (hasEffects && frameCounter % 60 == 0) {
        // Count active effects manually (cross-platform way)
        int activeCount = 0;
        uint32_t temp = m_enabledEffects;
        while (temp) {
            activeCount += temp & 1;
            temp >>= 1;
        }
        BGE_LOG_DEBUG("PostProcessor", "ProcessFrame completed with " + std::to_string(activeCount) + " active effects");
    }
    
    frameCounter++;
}

void PostProcessor::EnableEffect(PostProcessEffect effect) {
    m_enabledEffects |= static_cast<uint32_t>(effect);
    BGE_LOG_DEBUG("PostProcessor", "Enabled post-processing effect: " + std::to_string(static_cast<int>(effect)));
}

void PostProcessor::DisableEffect(PostProcessEffect effect) {
    m_enabledEffects &= ~static_cast<uint32_t>(effect);
    BGE_LOG_DEBUG("PostProcessor", "Disabled post-processing effect: " + std::to_string(static_cast<int>(effect)));
}

bool PostProcessor::IsEffectEnabled(PostProcessEffect effect) const {
    return (m_enabledEffects & static_cast<uint32_t>(effect)) != 0;
}

void PostProcessor::TriggerScreenShake(const ScreenShakeConfig& config) {
    m_shakeConfig = config;
    m_shakeTimeRemaining = config.duration;
    m_shakeTime = 0.0f;
    EnableEffect(PostProcessEffect::ScreenShake);
    
    BGE_LOG_DEBUG("PostProcessor", "Screen shake triggered - intensity: " + 
                  std::to_string(config.intensity) + ", duration: " + std::to_string(config.duration));
}

void PostProcessor::TriggerScreenShake(float intensity, float duration) {
    ScreenShakeConfig config;
    config.intensity = intensity;
    config.duration = duration;
    TriggerScreenShake(config);
}

void PostProcessor::Update(float deltaTime) {
    if (IsEffectEnabled(PostProcessEffect::ScreenShake)) {
        UpdateScreenShake(deltaTime);
    }
}

void PostProcessor::ApplyBloom(uint8_t* pixelData, int width, int height) {
    // Extract bright pixels for bloom
    for (int i = 0; i < width * height * 4; i += 4) {
        float brightness = GetPixelBrightness(pixelData[i], pixelData[i + 1], pixelData[i + 2]);
        
        if (brightness > m_bloomConfig.threshold) {
            // Scale brightness for bloom
            float bloomFactor = (brightness - m_bloomConfig.threshold) * m_bloomConfig.intensity;
            m_bloomBuffer[i] = static_cast<uint8_t>(std::min(255.0f, pixelData[i] * bloomFactor));
            m_bloomBuffer[i + 1] = static_cast<uint8_t>(std::min(255.0f, pixelData[i + 1] * bloomFactor));
            m_bloomBuffer[i + 2] = static_cast<uint8_t>(std::min(255.0f, pixelData[i + 2] * bloomFactor));
            m_bloomBuffer[i + 3] = pixelData[i + 3]; // Keep alpha
        } else {
            // Clear non-bright pixels
            m_bloomBuffer[i] = 0;
            m_bloomBuffer[i + 1] = 0;
            m_bloomBuffer[i + 2] = 0;
            m_bloomBuffer[i + 3] = 0;
        }
    }

    // Apply blur to bloom buffer
    for (int pass = 0; pass < m_bloomConfig.blurPasses; ++pass) {
        GaussianBlur(m_bloomBuffer.data(), width, height, m_bloomConfig.blurRadius);
    }

    // Combine bloom with original
    for (int i = 0; i < width * height * 4; i += 4) {
        if (pixelData[i + 3] > 0) { // Only process non-transparent pixels
            pixelData[i] = static_cast<uint8_t>(std::min(255.0f, pixelData[i] + m_bloomBuffer[i] * 0.3f));
            pixelData[i + 1] = static_cast<uint8_t>(std::min(255.0f, pixelData[i + 1] + m_bloomBuffer[i + 1] * 0.3f));
            pixelData[i + 2] = static_cast<uint8_t>(std::min(255.0f, pixelData[i + 2] + m_bloomBuffer[i + 2] * 0.3f));
        }
    }
}

void PostProcessor::ApplyColorGrading(uint8_t* pixelData, int width, int height) {
    for (int i = 0; i < width * height * 4; i += 4) {
        if (pixelData[i + 3] > 0) { // Only process non-transparent pixels
            Vector3 color = {
                pixelData[i] / 255.0f,
                pixelData[i + 1] / 255.0f,
                pixelData[i + 2] / 255.0f
            };

            Vector3 gradedColor = ApplyColorGradeToPixel(color);

            pixelData[i] = static_cast<uint8_t>(std::clamp(gradedColor.x * 255.0f, 0.0f, 255.0f));
            pixelData[i + 1] = static_cast<uint8_t>(std::clamp(gradedColor.y * 255.0f, 0.0f, 255.0f));
            pixelData[i + 2] = static_cast<uint8_t>(std::clamp(gradedColor.z * 255.0f, 0.0f, 255.0f));
        }
    }
}

void PostProcessor::ApplyScanlines(uint8_t* pixelData, int width, int height) {
    // Apply horizontal scanlines every 2 pixels for retro effect (much more obvious)
    for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 4;
            if (index + 3 < width * height * 4 && pixelData[index + 3] > 0) {
                // Make scanlines much more noticeable
                pixelData[index] = static_cast<uint8_t>(pixelData[index] * 0.3f);
                pixelData[index + 1] = static_cast<uint8_t>(pixelData[index + 1] * 0.3f);
                pixelData[index + 2] = static_cast<uint8_t>(pixelData[index + 2] * 0.3f);
            }
        }
    }
}

void PostProcessor::ApplyPixelation(uint8_t* pixelData, int width, int height) {
    // Simple pixelation by averaging 2x2 blocks
    const int blockSize = 2;
    
    for (int y = 0; y < height; y += blockSize) {
        for (int x = 0; x < width; x += blockSize) {
            int r = 0, g = 0, b = 0, a = 0, count = 0;
            
            // Average the block
            for (int by = 0; by < blockSize && y + by < height; ++by) {
                for (int bx = 0; bx < blockSize && x + bx < width; ++bx) {
                    int index = ((y + by) * width + (x + bx)) * 4;
                    if (pixelData[index + 3] > 0) {
                        r += pixelData[index];
                        g += pixelData[index + 1];
                        b += pixelData[index + 2];
                        a += pixelData[index + 3];
                        count++;
                    }
                }
            }
            
            if (count > 0) {
                uint8_t avgR = static_cast<uint8_t>(r / count);
                uint8_t avgG = static_cast<uint8_t>(g / count);
                uint8_t avgB = static_cast<uint8_t>(b / count);
                uint8_t avgA = static_cast<uint8_t>(a / count);
                
                // Apply average to all pixels in block
                for (int by = 0; by < blockSize && y + by < height; ++by) {
                    for (int bx = 0; bx < blockSize && x + bx < width; ++bx) {
                        int index = ((y + by) * width + (x + bx)) * 4;
                        pixelData[index] = avgR;
                        pixelData[index + 1] = avgG;
                        pixelData[index + 2] = avgB;
                        pixelData[index + 3] = avgA;
                    }
                }
            }
        }
    }
}

void PostProcessor::UpdateScreenShake(float deltaTime) {
    if (m_shakeTimeRemaining <= 0.0f) {
        DisableEffect(PostProcessEffect::ScreenShake);
        m_shakeOffset = {0.0f, 0.0f};
        return;
    }

    m_shakeTime += deltaTime;
    m_shakeTimeRemaining -= deltaTime;

    m_shakeOffset = CalculateShakeOffset();
}

Vector2 PostProcessor::CalculateShakeOffset() {
    if (m_shakeTimeRemaining <= 0.0f) {
        return {0.0f, 0.0f};
    }

    float intensity = m_shakeConfig.intensity;
    if (m_shakeConfig.decay) {
        intensity *= (m_shakeTimeRemaining / m_shakeConfig.duration);
    }

    // Use sine waves for smooth shake
    float shakeX = std::sin(m_shakeTime * m_shakeConfig.frequency) * intensity;
    float shakeY = std::cos(m_shakeTime * m_shakeConfig.frequency * 1.3f) * intensity;

    return {shakeX, shakeY};
}

float PostProcessor::GetPixelBrightness(uint8_t r, uint8_t g, uint8_t b) {
    // Luminance calculation
    return (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
}

Vector3 PostProcessor::ApplyColorGradeToPixel(const Vector3& color) {
    // Simple color grading implementation
    Vector3 result = color;

    // Apply contrast
    result.x = (result.x - 0.5f) * m_colorGradingConfig.contrast + 0.5f;
    result.y = (result.y - 0.5f) * m_colorGradingConfig.contrast + 0.5f;
    result.z = (result.z - 0.5f) * m_colorGradingConfig.contrast + 0.5f;

    // Apply saturation
    float gray = result.x * 0.299f + result.y * 0.587f + result.z * 0.114f;
    result.x = gray + (result.x - gray) * m_colorGradingConfig.saturation;
    result.y = gray + (result.y - gray) * m_colorGradingConfig.saturation;
    result.z = gray + (result.z - gray) * m_colorGradingConfig.saturation;

    // Apply tone mapping (simplified)
    float brightness = GetPixelBrightness(
        static_cast<uint8_t>(result.x * 255),
        static_cast<uint8_t>(result.y * 255),
        static_cast<uint8_t>(result.z * 255)
    );

    if (brightness < 0.33f) {
        // Shadows
        result.x *= m_colorGradingConfig.shadows.x;
        result.y *= m_colorGradingConfig.shadows.y;
        result.z *= m_colorGradingConfig.shadows.z;
    } else if (brightness < 0.66f) {
        // Midtones
        result.x *= m_colorGradingConfig.midtones.x;
        result.y *= m_colorGradingConfig.midtones.y;
        result.z *= m_colorGradingConfig.midtones.z;
    } else {
        // Highlights
        result.x *= m_colorGradingConfig.highlights.x;
        result.y *= m_colorGradingConfig.highlights.y;
        result.z *= m_colorGradingConfig.highlights.z;
    }

    return result;
}

void PostProcessor::GaussianBlur(uint8_t* buffer, int width, int height, float radius) {
    // Simple box blur approximation for performance
    int r = static_cast<int>(radius);
    if (r <= 0) return;

    // Copy to temp buffer
    std::copy(buffer, buffer + width * height * 4, m_tempBuffer.begin());

    // Horizontal pass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int rSum = 0, gSum = 0, bSum = 0, count = 0;
            
            for (int dx = -r; dx <= r; ++dx) {
                int nx = x + dx;
                if (nx >= 0 && nx < width) {
                    int index = (y * width + nx) * 4;
                    if (m_tempBuffer[index + 3] > 0) {
                        rSum += m_tempBuffer[index];
                        gSum += m_tempBuffer[index + 1];
                        bSum += m_tempBuffer[index + 2];
                        count++;
                    }
                }
            }
            
            if (count > 0) {
                int index = (y * width + x) * 4;
                buffer[index] = static_cast<uint8_t>(rSum / count);
                buffer[index + 1] = static_cast<uint8_t>(gSum / count);
                buffer[index + 2] = static_cast<uint8_t>(bSum / count);
            }
        }
    }

    // Copy back to temp
    std::copy(buffer, buffer + width * height * 4, m_tempBuffer.begin());

    // Vertical pass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int rSum = 0, gSum = 0, bSum = 0, count = 0;
            
            for (int dy = -r; dy <= r; ++dy) {
                int ny = y + dy;
                if (ny >= 0 && ny < height) {
                    int index = (ny * width + x) * 4;
                    if (m_tempBuffer[index + 3] > 0) {
                        rSum += m_tempBuffer[index];
                        gSum += m_tempBuffer[index + 1];
                        bSum += m_tempBuffer[index + 2];
                        count++;
                    }
                }
            }
            
            if (count > 0) {
                int index = (y * width + x) * 4;
                buffer[index] = static_cast<uint8_t>(rSum / count);
                buffer[index + 1] = static_cast<uint8_t>(gSum / count);
                buffer[index + 2] = static_cast<uint8_t>(bSum / count);
            }
        }
    }
}

} // namespace BGE
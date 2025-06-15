#pragma once

#include "../Core/Math/Vector2.h"
#include "../Core/Math/Vector3.h"
#include <vector>
#include <memory>

namespace BGE {

// Forward declarations
class PixelCamera;

// Post-processing effect types
enum class PostProcessEffect {
    None = 0,
    Bloom = 1 << 0,
    ScreenShake = 1 << 1,
    ColorGrading = 1 << 2,
    Scanlines = 1 << 3,
    Pixelation = 1 << 4
};

// Screen shake configuration
struct ScreenShakeConfig {
    float intensity = 1.0f;     // Shake strength multiplier
    float duration = 0.5f;      // How long the shake lasts
    float frequency = 30.0f;    // Shake frequency (Hz)
    bool decay = true;          // Whether intensity decays over time
};

// Bloom configuration
struct BloomConfig {
    float threshold = 0.8f;     // Brightness threshold for bloom
    float intensity = 1.5f;     // Bloom effect intensity
    int blurPasses = 3;         // Number of blur iterations
    float blurRadius = 2.0f;    // Blur kernel radius
};

// Color grading configuration
struct ColorGradingConfig {
    Vector3 shadows = {1.0f, 1.0f, 1.0f};      // Shadow color multiplier
    Vector3 midtones = {1.0f, 1.0f, 1.0f};     // Midtone color multiplier
    Vector3 highlights = {1.0f, 1.0f, 1.0f};   // Highlight color multiplier
    float contrast = 1.0f;                      // Contrast adjustment
    float saturation = 1.0f;                    // Saturation adjustment
};

class PostProcessor {
public:
    PostProcessor();
    ~PostProcessor();

    bool Initialize(int screenWidth, int screenHeight);
    void Shutdown();

    // Apply post-processing effects to pixel buffer
    void ProcessFrame(uint8_t* pixelData, int width, int height);

    // Effect management
    void EnableEffect(PostProcessEffect effect);
    void DisableEffect(PostProcessEffect effect);
    bool IsEffectEnabled(PostProcessEffect effect) const;

    // Screen shake
    void TriggerScreenShake(const ScreenShakeConfig& config);
    void TriggerScreenShake(float intensity, float duration);
    Vector2 GetShakeOffset() const { return m_shakeOffset; }

    // Configuration
    void SetBloomConfig(const BloomConfig& config) { m_bloomConfig = config; }
    void SetColorGradingConfig(const ColorGradingConfig& config) { m_colorGradingConfig = config; }
    const BloomConfig& GetBloomConfig() const { return m_bloomConfig; }
    const ColorGradingConfig& GetColorGradingConfig() const { return m_colorGradingConfig; }

    // Update for time-based effects
    void Update(float deltaTime);

private:
    // Effect implementations
    void ApplyBloom(uint8_t* pixelData, int width, int height);
    void ApplyColorGrading(uint8_t* pixelData, int width, int height);
    void ApplyScanlines(uint8_t* pixelData, int width, int height);
    void ApplyPixelation(uint8_t* pixelData, int width, int height);

    // Screen shake implementation
    void UpdateScreenShake(float deltaTime);
    Vector2 CalculateShakeOffset();

    // Utility functions
    float GetPixelBrightness(uint8_t r, uint8_t g, uint8_t b);
    Vector3 ApplyColorGradeToPixel(const Vector3& color);
    void GaussianBlur(uint8_t* buffer, int width, int height, float radius);

    // State
    uint32_t m_enabledEffects = 0;  // Bitmask of enabled effects
    int m_screenWidth = 0;
    int m_screenHeight = 0;

    // Screen shake state
    ScreenShakeConfig m_shakeConfig;
    float m_shakeTimeRemaining = 0.0f;
    float m_shakeTime = 0.0f;
    Vector2 m_shakeOffset = {0.0f, 0.0f};

    // Effect configurations
    BloomConfig m_bloomConfig;
    ColorGradingConfig m_colorGradingConfig;

    // Working buffers for effects
    std::vector<uint8_t> m_bloomBuffer;
    std::vector<uint8_t> m_tempBuffer;
};

// Inline helper for effect flags
inline PostProcessEffect operator|(PostProcessEffect a, PostProcessEffect b) {
    return static_cast<PostProcessEffect>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline PostProcessEffect operator&(PostProcessEffect a, PostProcessEffect b) {
    return static_cast<PostProcessEffect>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

} // namespace BGE
#pragma once

#include <string>
#include <filesystem>
#include <cstdint>

namespace BGE {

struct Texture {
    std::string path;
    int width = 0;
    int height = 0;
    int channels = 0;
    uint32_t rendererId = 0; // GPU texture ID
    std::filesystem::file_time_type lastWriteTime;
    
    // Optional: texture parameters
    bool mipmaps = false;
    bool linearFiltering = true;
    bool clampToEdge = false;
};

} // namespace BGE
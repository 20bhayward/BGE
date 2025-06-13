#pragma once

#include <string>
#include <filesystem> // Required for std::filesystem::file_time_type
#include <cstdint>    // Required for uint32_t

namespace BGE {

struct Texture {
    uint32_t rendererId; // The handle from the graphics API (e.g., OpenGL texture ID)
    std::string path;
    int width;
    int height;
    int channels;
    // The last-modified timestamp of the file when it was loaded
    std::filesystem::file_time_type lastWriteTime;
};

} // namespace BGE

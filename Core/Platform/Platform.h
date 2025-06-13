#pragma once

namespace BGE {

// Platform detection
#ifdef _WIN32
    #define BGE_PLATFORM_WINDOWS
    #ifdef _WIN64
        #define BGE_PLATFORM_WINDOWS_64
    #else
        #define BGE_PLATFORM_WINDOWS_32
    #endif
#elif defined(__linux__)
    #define BGE_PLATFORM_LINUX
#elif defined(__APPLE__)
    #define BGE_PLATFORM_MACOS
#else
    #define BGE_PLATFORM_UNKNOWN
#endif

// Platform-specific types and functions
namespace Platform {
    // File system
    bool FileExists(const char* path);
    bool CreateDirectory(const char* path);
    bool DeleteFile(const char* path);
    
    // Time
    double GetHighResolutionTime();
    void Sleep(uint32_t milliseconds);
    
    // System info
    uint32_t GetCoreCount();
    uint64_t GetTotalMemory();
    uint64_t GetAvailableMemory();
    
    // Debug
    void DebugBreak();
    void OutputDebugString(const char* message);
}

} // namespace BGE
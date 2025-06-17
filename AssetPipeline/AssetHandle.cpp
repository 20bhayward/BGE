#include "AssetHandle.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace BGE {

AssetHandle AssetHandle::Generate() {
    // Generate a simple UUID-like string
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    const char* hexChars = "0123456789abcdef";
    
    // Generate format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    for (int i = 0; i < 32; ++i) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            ss << '-';
        }
        if (i == 12) {
            ss << '4'; // Version 4 UUID
        } else if (i == 16) {
            ss << hexChars[(dis(gen) & 0x3) | 0x8]; // Variant bits
        } else {
            ss << hexChars[dis(gen)];
        }
    }
    
    return AssetHandle(ss.str());
}

AssetHandle AssetHandle::FromString(const std::string& uuidStr) {
    return AssetHandle(uuidStr);
}

} // namespace BGE
#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace BGE {

using ConfigValue = std::variant<bool, int, float, std::string>;

class ConfigManager {
public:
    static ConfigManager& Instance();
    
    bool LoadFromFile(const std::string& filePath);
    void SaveToFile(const std::string& filePath) const;
    
    void SetBool(const std::string& key, bool value);
    void SetInt(const std::string& key, int value);
    void SetFloat(const std::string& key, float value);
    void SetString(const std::string& key, const std::string& value);
    
    bool GetBool(const std::string& key, bool defaultValue = false) const;
    int GetInt(const std::string& key, int defaultValue = 0) const;
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    
    bool HasKey(const std::string& key) const;
    void RemoveKey(const std::string& key);
    void Clear();
    
    std::vector<std::string> GetAllKeys() const;
    
private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    
    std::unordered_map<std::string, ConfigValue> m_config;
    
    std::string Trim(const std::string& str) const;
    std::pair<std::string, std::string> ParseLine(const std::string& line) const;
    ConfigValue ParseValue(const std::string& value) const;
    std::string ValueToString(const ConfigValue& value) const;
};

} // namespace BGE
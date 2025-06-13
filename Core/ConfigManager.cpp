#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace BGE {

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        auto [key, value] = ParseLine(line);
        if (!key.empty()) {
            m_config[key] = ParseValue(value);
        }
    }
    
    return true;
}

void ConfigManager::SaveToFile(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return;
    }
    
    for (const auto& [key, value] : m_config) {
        file << key << " = " << ValueToString(value) << std::endl;
    }
}

void ConfigManager::SetBool(const std::string& key, bool value) {
    m_config[key] = value;
}

void ConfigManager::SetInt(const std::string& key, int value) {
    m_config[key] = value;
}

void ConfigManager::SetFloat(const std::string& key, float value) {
    m_config[key] = value;
}

void ConfigManager::SetString(const std::string& key, const std::string& value) {
    m_config[key] = value;
}

bool ConfigManager::GetBool(const std::string& key, bool defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end() && std::holds_alternative<bool>(it->second)) {
        return std::get<bool>(it->second);
    }
    return defaultValue;
}

int ConfigManager::GetInt(const std::string& key, int defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end() && std::holds_alternative<int>(it->second)) {
        return std::get<int>(it->second);
    }
    return defaultValue;
}

float ConfigManager::GetFloat(const std::string& key, float defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end() && std::holds_alternative<float>(it->second)) {
        return std::get<float>(it->second);
    }
    return defaultValue;
}

std::string ConfigManager::GetString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end() && std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    return defaultValue;
}

bool ConfigManager::HasKey(const std::string& key) const {
    return m_config.find(key) != m_config.end();
}

void ConfigManager::RemoveKey(const std::string& key) {
    m_config.erase(key);
}

void ConfigManager::Clear() {
    m_config.clear();
}

std::vector<std::string> ConfigManager::GetAllKeys() const {
    std::vector<std::string> keys;
    keys.reserve(m_config.size());
    for (const auto& [key, value] : m_config) {
        keys.push_back(key);
    }
    return keys;
}

std::string ConfigManager::Trim(const std::string& str) const {
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(str[start])) {
        ++start;
    }
    
    while (end > start && std::isspace(str[end - 1])) {
        --end;
    }
    
    return str.substr(start, end - start);
}

std::pair<std::string, std::string> ConfigManager::ParseLine(const std::string& line) const {
    size_t pos = line.find('=');
    if (pos == std::string::npos) {
        return {"", ""};
    }
    
    std::string key = Trim(line.substr(0, pos));
    std::string value = Trim(line.substr(pos + 1));
    
    return {key, value};
}

ConfigValue ConfigManager::ParseValue(const std::string& value) const {
    if (value.empty()) {
        return std::string("");
    }
    
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "true") {
        return true;
    } else if (lower == "false") {
        return false;
    }
    
    if (value.find('.') != std::string::npos) {
        try {
            return std::stof(value);
        } catch (...) {
            return value;
        }
    }
    
    try {
        return std::stoi(value);
    } catch (...) {
        return value;
    }
}

std::string ConfigManager::ValueToString(const ConfigValue& value) const {
    return std::visit([](const auto& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, bool>) {
            return val ? "true" : "false";
        } else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(val);
        } else if constexpr (std::is_same_v<T, float>) {
            return std::to_string(val);
        } else {
            return val;
        }
    }, value);
}

} // namespace BGE
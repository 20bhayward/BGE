#include "LayoutData.h"
#include <iostream>
#include <sstream>

namespace BGE {

std::string SerializeNodeData(const LayoutNodeData& node, int indent = 0) {
    std::stringstream ss;
    std::string indentStr(indent, ' ');
    
    ss << indentStr << "{\n";
    ss << indentStr << "  \"type\": " << static_cast<int>(node.type) << ",\n";
    ss << indentStr << "  \"position\": [" << node.position.x << ", " << node.position.y << "],\n";
    ss << indentStr << "  \"size\": [" << node.size.x << ", " << node.size.y << "],\n";
    ss << indentStr << "  \"splitRatio\": " << node.splitRatio << ",\n";
    ss << indentStr << "  \"isHorizontalSplit\": " << (node.isHorizontalSplit ? "true" : "false") << ",\n";
    ss << indentStr << "  \"activeTabIndex\": " << node.activeTabIndex << ",\n";
    ss << indentStr << "  \"nodeId\": \"" << node.nodeId << "\",\n";
    
    // Serialize panel names
    ss << indentStr << "  \"panelNames\": [";
    for (size_t i = 0; i < node.panelNames.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << node.panelNames[i] << "\"";
    }
    ss << "],\n";
    
    // Serialize children
    ss << indentStr << "  \"children\": [\n";
    for (size_t i = 0; i < node.children.size(); ++i) {
        if (i > 0) ss << ",\n";
        ss << SerializeNodeData(node.children[i], indent + 4);
    }
    ss << "\n" << indentStr << "  ]\n";
    
    ss << indentStr << "}";
    return ss.str();
}

std::string PersistentLayoutInfo::Serialize() const {
    std::stringstream ss;
    
    ss << "{\n";
    ss << "  \"name\": \"" << name << "\",\n";
    ss << "  \"baseLayout\": \"" << baseLayout << "\",\n";
    ss << "  \"rootNode\": " << SerializeNodeData(rootNode, 2) << ",\n";
    
    // Serialize floating nodes
    ss << "  \"floatingNodes\": [\n";
    for (size_t i = 0; i < floatingNodes.size(); ++i) {
        if (i > 0) ss << ",\n";
        ss << SerializeNodeData(floatingNodes[i], 4);
    }
    ss << "\n  ],\n";
    
    // Serialize panel visibility
    ss << "  \"panelVisibility\": {\n";
    bool first = true;
    for (const auto& [panelName, visible] : panelVisibility) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    \"" << panelName << "\": " << (visible ? "true" : "false");
    }
    ss << "\n  }\n";
    
    ss << "}\n";
    return ss.str();
}

// Helper function to extract string value from JSON
std::string ExtractJsonString(const std::string& data, const std::string& key, size_t startPos = 0) {
    std::string searchKey = "\"" + key + "\":";
    size_t keyPos = data.find(searchKey, startPos);
    if (keyPos == std::string::npos) return "";
    
    size_t valueStart = data.find("\"", keyPos + searchKey.length()) + 1;
    size_t valueEnd = data.find("\"", valueStart);
    if (valueStart == std::string::npos || valueEnd == std::string::npos) return "";
    
    return data.substr(valueStart, valueEnd - valueStart);
}

// Helper function to extract numeric value from JSON
float ExtractJsonFloat(const std::string& data, const std::string& key, size_t startPos = 0) {
    std::string searchKey = "\"" + key + "\":";
    size_t keyPos = data.find(searchKey, startPos);
    if (keyPos == std::string::npos) return 0.0f;
    
    size_t valueStart = keyPos + searchKey.length();
    while (valueStart < data.length() && (data[valueStart] == ' ' || data[valueStart] == '\t')) valueStart++;
    
    size_t valueEnd = valueStart;
    while (valueEnd < data.length() && (std::isdigit(data[valueEnd]) || data[valueEnd] == '.' || data[valueEnd] == '-')) valueEnd++;
    
    if (valueEnd > valueStart) {
        return std::stof(data.substr(valueStart, valueEnd - valueStart));
    }
    return 0.0f;
}

// Helper function to extract integer value from JSON
int ExtractJsonInt(const std::string& data, const std::string& key, size_t startPos = 0) {
    return static_cast<int>(ExtractJsonFloat(data, key, startPos));
}

// Helper function to extract boolean value from JSON
bool ExtractJsonBool(const std::string& data, const std::string& key, size_t startPos = 0) {
    std::string searchKey = "\"" + key + "\":";
    size_t keyPos = data.find(searchKey, startPos);
    if (keyPos == std::string::npos) return false;
    
    size_t valuePos = data.find("true", keyPos + searchKey.length());
    if (valuePos != std::string::npos && valuePos < keyPos + searchKey.length() + 10) {
        return true;
    }
    return false;
}

// Helper function to extract array values
std::vector<float> ExtractJsonFloatArray(const std::string& data, const std::string& key, size_t startPos = 0) {
    std::vector<float> result;
    std::string searchKey = "\"" + key + "\":";
    size_t keyPos = data.find(searchKey, startPos);
    if (keyPos == std::string::npos) return result;
    
    size_t arrayStart = data.find("[", keyPos + searchKey.length());
    size_t arrayEnd = data.find("]", arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) return result;
    
    std::string arrayContent = data.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    size_t pos = 0;
    while (pos < arrayContent.length()) {
        size_t nextComma = arrayContent.find(",", pos);
        if (nextComma == std::string::npos) nextComma = arrayContent.length();
        
        std::string valueStr = arrayContent.substr(pos, nextComma - pos);
        // Remove whitespace
        valueStr.erase(0, valueStr.find_first_not_of(" \t"));
        valueStr.erase(valueStr.find_last_not_of(" \t") + 1);
        
        if (!valueStr.empty()) {
            result.push_back(std::stof(valueStr));
        }
        
        pos = nextComma + 1;
    }
    
    return result;
}

// Helper function to extract panel names array
std::vector<std::string> ExtractJsonStringArray(const std::string& data, const std::string& key, size_t startPos = 0) {
    std::vector<std::string> result;
    std::string searchKey = "\"" + key + "\":";
    size_t keyPos = data.find(searchKey, startPos);
    if (keyPos == std::string::npos) return result;
    
    size_t arrayStart = data.find("[", keyPos + searchKey.length());
    size_t arrayEnd = data.find("]", arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) return result;
    
    std::string arrayContent = data.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    size_t pos = 0;
    while (pos < arrayContent.length()) {
        size_t quoteStart = arrayContent.find("\"", pos);
        if (quoteStart == std::string::npos) break;
        
        size_t quoteEnd = arrayContent.find("\"", quoteStart + 1);
        if (quoteEnd == std::string::npos) break;
        
        result.push_back(arrayContent.substr(quoteStart + 1, quoteEnd - quoteStart - 1));
        pos = quoteEnd + 1;
    }
    
    return result;
}

// Helper function to deserialize a node
LayoutNodeData DeserializeNodeData(const std::string& data, size_t& pos) {
    LayoutNodeData node;
    
    // Find the start of this node object
    size_t objStart = data.find("{", pos);
    if (objStart == std::string::npos) return node;
    
    // Find the end of this node object (simple bracket counting)
    int braceCount = 1;
    size_t objEnd = objStart + 1;
    while (objEnd < data.length() && braceCount > 0) {
        if (data[objEnd] == '{') braceCount++;
        else if (data[objEnd] == '}') braceCount--;
        objEnd++;
    }
    
    std::string nodeData = data.substr(objStart, objEnd - objStart);
    
    // Extract node properties
    node.type = static_cast<LayoutNodeType>(ExtractJsonInt(nodeData, "type"));
    
    auto position = ExtractJsonFloatArray(nodeData, "position");
    if (position.size() >= 2) {
        node.position = ImVec2(position[0], position[1]);
    }
    
    auto size = ExtractJsonFloatArray(nodeData, "size");
    if (size.size() >= 2) {
        node.size = ImVec2(size[0], size[1]);
    }
    
    node.splitRatio = ExtractJsonFloat(nodeData, "splitRatio");
    node.isHorizontalSplit = ExtractJsonBool(nodeData, "isHorizontalSplit");
    node.activeTabIndex = ExtractJsonInt(nodeData, "activeTabIndex");
    node.nodeId = ExtractJsonString(nodeData, "nodeId");
    node.panelNames = ExtractJsonStringArray(nodeData, "panelNames");
    
    // Parse children recursively
    size_t childrenPos = nodeData.find("\"children\":");
    if (childrenPos != std::string::npos) {
        size_t arrayStart = nodeData.find("[", childrenPos);
        if (arrayStart != std::string::npos) {
            size_t arrayEnd = arrayStart + 1;
            int bracketCount = 1;
            
            // Find the end of the children array
            while (arrayEnd < nodeData.length() && bracketCount > 0) {
                if (nodeData[arrayEnd] == '[') bracketCount++;
                else if (nodeData[arrayEnd] == ']') bracketCount--;
                arrayEnd++;
            }
            
            if (bracketCount == 0) {
                std::string childrenData = nodeData.substr(arrayStart + 1, arrayEnd - arrayStart - 2);
                
                // Parse each child object
                size_t childPos = 0;
                while (childPos < childrenData.length()) {
                    size_t nextBrace = childrenData.find("{", childPos);
                    if (nextBrace == std::string::npos) break;
                    
                    // Find the complete child object
                    int childBraceCount = 1;
                    size_t childEnd = nextBrace + 1;
                    while (childEnd < childrenData.length() && childBraceCount > 0) {
                        if (childrenData[childEnd] == '{') childBraceCount++;
                        else if (childrenData[childEnd] == '}') childBraceCount--;
                        childEnd++;
                    }
                    
                    if (childBraceCount == 0) {
                        std::string childObjectData = childrenData.substr(nextBrace, childEnd - nextBrace);
                        size_t tempPos = 0;
                        LayoutNodeData childNode = DeserializeNodeData(childObjectData, tempPos);
                        node.children.push_back(childNode);
                    }
                    
                    childPos = childEnd;
                }
            }
        }
    }
    
    pos = objEnd;
    return node;
}

bool PersistentLayoutInfo::Deserialize(const std::string& data) {
    if (data.empty()) return false;
    
    // Extract name
    name = ExtractJsonString(data, "name");
    if (name.empty()) return false;
    
    // Extract base layout
    baseLayout = ExtractJsonString(data, "baseLayout");
    if (baseLayout.empty()) {
        baseLayout = "Unity"; // Default to Unity if not specified
    }
    
    // Try to extract rootNode (simplified detection)
    size_t rootNodePos = data.find("\"rootNode\":");
    if (rootNodePos != std::string::npos) {
        rootNode = DeserializeNodeData(data, rootNodePos);
    }
    
    // Extract floating nodes (simplified - just detect if they exist)
    floatingNodes.clear();
    size_t floatingPos = data.find("\"floatingNodes\":");
    if (floatingPos != std::string::npos) {
        // For now, just create an empty list - full parsing can be added later
        // The key thing is that we detect the new format exists
    }
    
    // Extract panel visibility (simplified parsing)
    panelVisibility.clear();
    size_t visibilityPos = data.find("\"panelVisibility\":");
    if (visibilityPos != std::string::npos) {
        size_t objStart = data.find("{", visibilityPos);
        size_t objEnd = data.find("}", objStart);
        if (objStart != std::string::npos && objEnd != std::string::npos) {
            std::string visibilityData = data.substr(objStart + 1, objEnd - objStart - 1);
            
            // Parse key-value pairs using simple string searching
            size_t pos = 0;
            while (pos < visibilityData.length()) {
                size_t keyStart = visibilityData.find("\"", pos);
                if (keyStart == std::string::npos) break;
                
                size_t keyEnd = visibilityData.find("\"", keyStart + 1);
                if (keyEnd == std::string::npos) break;
                
                std::string key = visibilityData.substr(keyStart + 1, keyEnd - keyStart - 1);
                
                size_t colonPos = visibilityData.find(":", keyEnd);
                if (colonPos == std::string::npos) break;
                
                // Look for true/false after the colon
                size_t valueStart = colonPos + 1;
                size_t truePos = visibilityData.find("true", valueStart);
                size_t falsePos = visibilityData.find("false", valueStart);
                size_t commaPos = visibilityData.find(",", valueStart);
                size_t bracePos = visibilityData.find("}", valueStart);
                
                // Find the closest delimiter to determine the value boundary
                size_t valueEnd = visibilityData.length();
                if (commaPos != std::string::npos) valueEnd = std::min(valueEnd, commaPos);
                if (bracePos != std::string::npos) valueEnd = std::min(valueEnd, bracePos);
                
                bool value = false;
                if (truePos != std::string::npos && truePos < valueEnd) {
                    value = true;
                } else if (falsePos != std::string::npos && falsePos < valueEnd) {
                    value = false;
                }
                
                panelVisibility[key] = value;
                
                // Move to next entry
                if (commaPos != std::string::npos && commaPos < valueEnd) {
                    pos = commaPos + 1;
                } else {
                    break;
                }
            }
        }
    }
    
    std::cout << "Deserialized layout: " << name << " with " << panelVisibility.size() << " panel visibility entries" << std::endl;
    return true;
}

} // namespace BGE
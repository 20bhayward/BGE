#pragma once

#include "DockNode.h"
#include <string>
#include <memory>

namespace BGE {

class LayoutSerializer {
public:
    // Serialize dock tree to JSON
    static std::string SerializeLayout(std::shared_ptr<DockNode> rootNode);
    
    // Deserialize dock tree from JSON
    static std::shared_ptr<DockNode> DeserializeLayout(const std::string& jsonData);
    
    // Save layout to file
    static bool SaveLayoutToFile(std::shared_ptr<DockNode> rootNode, const std::string& filename);
    
    // Load layout from file
    static std::shared_ptr<DockNode> LoadLayoutFromFile(const std::string& filename);

private:
    // Internal serialization methods (implemented in .cpp file)
    // Using string-based serialization to avoid JSON header inclusion
    static std::string SerializeNodeToString(std::shared_ptr<DockNode> node);
    static std::shared_ptr<DockNode> DeserializeNodeFromString(const std::string& data);
};

} // namespace BGE
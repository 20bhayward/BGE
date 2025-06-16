#include "LayoutSerializer.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace BGE {

std::string LayoutSerializer::SerializeLayout(std::shared_ptr<DockNode> rootNode) {
    if (!rootNode) return "";
    
    std::stringstream ss;
    ss << "BGE_LAYOUT_V1\n";
    ss << SerializeNodeToString(rootNode);
    return ss.str();
}

std::shared_ptr<DockNode> LayoutSerializer::DeserializeLayout(const std::string& data) {
    if (data.empty() || data.find("BGE_LAYOUT_V1") != 0) {
        return nullptr;
    }
    
    // For now, just return nullptr - layout persistence can be added later
    // This is a placeholder to satisfy the interface
    std::cout << "Layout deserialization not yet implemented (complex feature)" << std::endl;
    return nullptr;
}

bool LayoutSerializer::SaveLayoutToFile(std::shared_ptr<DockNode> rootNode, const std::string& filename) {
    try {
        std::string data = SerializeLayout(rootNode);
        std::ofstream file(filename);
        if (file.is_open()) {
            file << data;
            file.close();
            std::cout << "Layout saved to: " << filename << std::endl;
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to save layout to file: " << e.what() << std::endl;
    }
    return false;
}

std::shared_ptr<DockNode> LayoutSerializer::LoadLayoutFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string data((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
            file.close();
            
            auto layout = DeserializeLayout(data);
            if (layout) {
                std::cout << "Layout loaded from: " << filename << std::endl;
            }
            return layout;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load layout from file: " << e.what() << std::endl;
    }
    return nullptr;
}

std::string LayoutSerializer::SerializeNodeToString(std::shared_ptr<DockNode> node) {
    if (!node) return "";
    
    std::stringstream ss;
    const auto& data = node->GetData();
    
    // Serialize basic node info
    ss << "Node:" << node->GetId() << "\n";
    ss << "Type:" << static_cast<int>(node->GetType()) << "\n";
    ss << "Pos:" << data.position.x << "," << data.position.y << "\n";
    ss << "Size:" << data.size.x << "," << data.size.y << "\n";
    ss << "Split:" << data.splitRatio << "," << (data.isHorizontalSplit ? 1 : 0) << "\n";
    ss << "Tab:" << data.activeTabIndex << "\n";
    
    // Serialize panel names for leaf nodes
    if (node->IsLeaf()) {
        ss << "Panels:";
        for (const auto& panel : node->GetPanels()) {
            if (panel) {
                ss << panel->GetName() << ",";
            }
        }
        ss << "\n";
    }
    
    // Serialize children for split nodes
    if (node->IsSplit()) {
        ss << "Children:" << node->GetChildren().size() << "\n";
        for (const auto& child : node->GetChildren()) {
            if (child) {
                ss << SerializeNodeToString(child);
            }
        }
    }
    
    ss << "EndNode\n";
    return ss.str();
}

std::shared_ptr<DockNode> LayoutSerializer::DeserializeNodeFromString(const std::string& /* data */) {
    // Placeholder implementation - full parsing would be complex
    // For now, just create a basic node
    return std::make_shared<DockNode>(DockNodeType::Leaf);
}

} // namespace BGE
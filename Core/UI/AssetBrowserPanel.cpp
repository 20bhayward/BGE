#include "AssetBrowserPanel.h"
#include "../Services.h"
#include "../ServiceLocator.h"
#include "../../Renderer/Renderer.h"
#include "../Events.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace BGE {

AssetBrowserPanel::AssetBrowserPanel(const std::string& name)
    : Panel(name, PanelDockPosition::Bottom) {
    
    // Set up project paths
    m_projectRoot = fs::current_path().string();
    m_assetsDirectory = m_projectRoot + "/Assets";
    m_currentDirectory = m_assetsDirectory;
    
    // Create Assets directory if it doesn't exist
    if (!fs::exists(m_assetsDirectory)) {
        fs::create_directories(m_assetsDirectory);
    }
    
    m_lastRefresh = std::chrono::steady_clock::now();
}

AssetBrowserPanel::~AssetBrowserPanel() {
    UnregisterEventListeners();
    StopFileSystemWatcher();
}

void AssetBrowserPanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoCollapse);
    RegisterEventListeners();
    RefreshCurrentDirectory();
    StartFileSystemWatcher();
}

void AssetBrowserPanel::RegisterEventListeners() {
    auto eventBusPtr = ServiceLocator::Instance().GetService<EventBus>();
    m_eventBus = eventBusPtr.get();
}

void AssetBrowserPanel::UnregisterEventListeners() {
    // EventBus will handle cleanup when it's destroyed
}

void AssetBrowserPanel::OnRender() {
    // Check for file system changes periodically
    CheckFileSystemChanges();
    
    RenderToolbar();
    RenderBreadcrumbs();
    
    // Split panel: directory tree on left, asset grid on right
    if (ImGui::BeginChild("AssetBrowserSplit", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar)) {
        // Left panel: Directory tree
        if (ImGui::BeginChild("DirectoryTree", ImVec2(m_leftPanelWidth, 0), true)) {
            RenderLeftPanel();
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Splitter
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.3f));
        ImGui::Button("##Splitter", ImVec2(4.0f, -1));
        ImGui::PopStyleColor();
        
        if (ImGui::IsItemActive()) {
            m_leftPanelWidth += ImGui::GetIO().MouseDelta.x;
            m_leftPanelWidth = std::max(100.0f, std::min(400.0f, m_leftPanelWidth));
        }
        
        ImGui::SameLine();
        
        // Right panel: Asset grid
        if (ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true)) {
            RenderMainPanel();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
    
    RenderStatusBar();
    
    // Handle context menus
    if (m_showCreateMenu) {
        RenderCreateContextMenu();
    }
    
    // Handle drag and drop
    HandleDragAndDrop();
}

void AssetBrowserPanel::RenderToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    
    // Navigation buttons
    if (ImGui::Button("🏠")) {
        NavigateToDirectory(m_assetsDirectory);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Assets Root");
    
    ImGui::SameLine();
    if (ImGui::Button("⬆")) {
        std::string parent = fs::path(m_currentDirectory).parent_path().string();
        if (parent.length() >= m_assetsDirectory.length()) {
            NavigateToDirectory(parent);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Parent Directory");
    
    ImGui::SameLine();
    if (ImGui::Button("🔄")) {
        RefreshCurrentDirectory();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Refresh");
    
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    // View options
    ImGui::SetNextItemWidth(80);
    if (ImGui::SliderFloat("##IconSize", &m_iconSize, 32.0f, 128.0f, "%.0f")) {
        m_gridColumns = std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x - m_leftPanelWidth) / (m_iconSize + 8)));
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Icon Size");
    
    ImGui::SameLine();
    ImGui::Checkbox("Hidden", &m_showHiddenFiles);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Hidden Files");
    
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    // Search
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Search", "Search assets...", m_searchBuffer, sizeof(m_searchBuffer))) {
        // Filter will be applied in RenderAssetGrid
    }
    
    ImGui::PopStyleVar();
}

void AssetBrowserPanel::RenderBreadcrumbs() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    
    // Build breadcrumb path
    fs::path currentPath(m_currentDirectory);
    fs::path assetsPath(m_assetsDirectory);
    
    std::vector<std::string> pathParts;
    pathParts.push_back("Assets");
    
    if (currentPath != assetsPath) {
        fs::path relativePath = fs::relative(currentPath, assetsPath);
        for (const auto& part : relativePath) {
            pathParts.push_back(part.string());
        }
    }
    
    // Render breadcrumbs
    for (size_t i = 0; i < pathParts.size(); ++i) {
        if (i > 0) {
            ImGui::SameLine();
            ImGui::Text(">");
            ImGui::SameLine();
        }
        
        if (ImGui::Button(pathParts[i].c_str())) {
            // Navigate to this breadcrumb level
            std::string targetPath = m_assetsDirectory;
            for (size_t j = 1; j <= i; ++j) {
                targetPath += "/" + pathParts[j];
            }
            NavigateToDirectory(targetPath);
        }
    }
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::Separator();
}

void AssetBrowserPanel::RenderLeftPanel() {
    ImGui::Text("F Project");
    ImGui::Separator();
    
    // Render directory tree starting from Assets
    RenderDirectoryTree(m_assetsDirectory);
}

void AssetBrowserPanel::RenderDirectoryTree(const std::string& path, int depth) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        return;
    }
    
    std::string folderName = fs::path(path).filename().string();
    if (folderName.empty()) folderName = "Assets";
    
    bool isExpanded = IsDirectoryExpanded(path);
    bool hasSubdirs = false;
    
    // Check if directory has subdirectories
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                hasSubdirs = true;
                break;
            }
        }
    } catch (const std::exception&) {
        // Ignore permission errors
    }
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (path == m_currentDirectory) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!hasSubdirs) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (isExpanded) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    
    bool nodeOpen = ImGui::TreeNodeEx(folderName.c_str(), flags);
    
    // Handle directory selection
    if (ImGui::IsItemClicked()) {
        NavigateToDirectory(path);
    }
    
    // Handle expansion state
    if (nodeOpen != isExpanded) {
        SetDirectoryExpanded(path, nodeOpen);
    }
    
    // Render subdirectories
    if (nodeOpen) {
        if (hasSubdirs) {
            try {
                std::vector<std::string> subdirs;
                for (const auto& entry : fs::directory_iterator(path)) {
                    if (entry.is_directory()) {
                        subdirs.push_back(entry.path().string());
                    }
                }
                
                std::sort(subdirs.begin(), subdirs.end());
                
                for (const std::string& subdir : subdirs) {
                    RenderDirectoryTree(subdir, depth + 1);
                }
            } catch (const std::exception&) {
                // Ignore permission errors
            }
        }
        
        // Always call TreePop() if TreeNodeEx() returned true
        ImGui::TreePop();
    }
}

void AssetBrowserPanel::RenderMainPanel() {
    // Context menu for empty space
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered()) {
        m_showCreateMenu = true;
        ImGui::OpenPopup("CreateAssetMenu");
    }
    
    RenderAssetGrid();
    
    // Handle keyboard shortcuts
    if (ImGui::IsWindowFocused()) {
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(261)) && !m_selectedAsset.empty()) { // 261 = Delete
            DeleteAsset(m_selectedAsset);
        }
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(291)) && !m_selectedAsset.empty()) { // 291 = F2
            m_renameMode = true;
            m_renamingAsset = m_selectedAsset;
            std::string name = fs::path(m_selectedAsset).stem().string();
            strncpy(m_renameBuffer, name.c_str(), sizeof(m_renameBuffer) - 1);
            m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
        }
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(static_cast<ImGuiKey>('D')) && !m_selectedAsset.empty()) {
            DuplicateAsset(m_selectedAsset);
        }
    }
}

void AssetBrowserPanel::RenderAssetGrid() {
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    m_gridColumns = std::max(1, static_cast<int>(contentRegion.x / (m_iconSize + 8)));
    
    int column = 0;
    
    for (size_t i = 0; i < m_currentAssets.size(); ++i) {
        const AssetInfo& asset = m_currentAssets[i];
        
        // Apply search filter
        if (strlen(m_searchBuffer) > 0) {
            std::string lowerName = asset.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::string lowerSearch = m_searchBuffer;
            std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
            
            if (lowerName.find(lowerSearch) == std::string::npos) {
                continue;
            }
        }
        
        // Skip hidden files
        if (!m_showHiddenFiles && asset.name[0] == '.') {
            continue;
        }
        
        // Render asset icon
        ImVec2 iconSize(m_iconSize, m_iconSize);
        RenderAssetIcon(asset, iconSize);
        
        // Handle grid layout
        column++;
        if (column < m_gridColumns && i < m_currentAssets.size() - 1) {
            ImGui::SameLine();
        } else {
            column = 0;
        }
    }
}

void AssetBrowserPanel::RenderAssetIcon(const AssetInfo& asset, const ImVec2& iconSize) {
    ImGui::PushID(asset.path.c_str());
    
    ImVec2 cursorPos = ImGui::GetCursorPos();
    
    // Selection highlighting
    bool isSelected = (asset.path == m_selectedAsset);
    if (isSelected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 0.5f));
    }
    
    // Render icon button
    std::string iconText = GetAssetIconText(asset.type);
    if (ImGui::Button(iconText.c_str(), iconSize)) {
        SelectAsset(asset.path);
        
        // Double-click to navigate into directories
        if (asset.isDirectory && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            NavigateToDirectory(asset.path);
        }
    }
    
    if (isSelected) {
        ImGui::PopStyleColor();
    }
    
    // Context menu
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        SelectAsset(asset.path);
        m_contextMenuAsset = asset.path;
        m_showAssetMenu = true;
        ImGui::OpenPopup("AssetContextMenu");
    }
    
    // Drag and drop source
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("ASSET_PATH", asset.path.c_str(), asset.path.length() + 1);
        ImGui::Text(">> %s", asset.name.c_str());
        ImGui::EndDragDropSource();
    }
    
    // Asset name label
    ImVec2 labelSize = ImGui::CalcTextSize(asset.name.c_str());
    float labelWidth = std::min(labelSize.x, iconSize.x);
    
    ImGui::SetCursorPos(ImVec2(cursorPos.x + (iconSize.x - labelWidth) * 0.5f, cursorPos.y + iconSize.y + 2));
    
    if (m_renameMode && asset.path == m_renamingAsset) {
        ImGui::SetNextItemWidth(iconSize.x);
        if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), 
                           ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            RenameAsset(asset.path, m_renameBuffer);
            m_renameMode = false;
            m_renamingAsset.clear();
        }
        
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(256)) || (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))) { // 256 = Escape
            m_renameMode = false;
            m_renamingAsset.clear();
        }
    } else {
        ImGui::TextWrapped("%s", asset.name.c_str());
    }
    
    // Render context menu for this asset
    if (m_showAssetMenu && m_contextMenuAsset == asset.path) {
        RenderAssetContextMenu(asset);
    }
    
    ImGui::PopID();
}

void AssetBrowserPanel::RenderAssetContextMenu(const AssetInfo& asset) {
    if (ImGui::BeginPopup("AssetContextMenu")) {
        m_showAssetMenu = false;
        
        ImGui::Text("📄 %s", asset.name.c_str());
        ImGui::Separator();
        
        if (ImGui::MenuItem("Open")) {
            // TODO: Open asset in appropriate editor
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Rename", "F2")) {
            m_renameMode = true;
            m_renamingAsset = asset.path;
            std::string name = fs::path(asset.path).stem().string();
            strncpy(m_renameBuffer, name.c_str(), sizeof(m_renameBuffer) - 1);
            m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
        }
        
        if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
            DuplicateAsset(asset.path);
        }
        
        if (ImGui::MenuItem("Delete", "Del")) {
            DeleteAsset(asset.path);
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Show in Explorer")) {
            // TODO: Open file explorer to this asset
        }
        
        ImGui::EndPopup();
    } else {
        m_showAssetMenu = false;
    }
}

void AssetBrowserPanel::RenderCreateContextMenu() {
    if (ImGui::BeginPopup("CreateAssetMenu")) {
        m_showCreateMenu = false;
        
        ImGui::Text("Create");
        ImGui::Separator();
        
        if (ImGui::MenuItem("F Folder")) {
            CreateFolder("New Folder");
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("M Material")) {
            CreateAsset("New Material.json", AssetType::Material);
        }
        
        if (ImGui::MenuItem("S Scene")) {
            CreateAsset("New Scene.json", AssetType::Scene);
        }
        
        if (ImGui::MenuItem("P Prefab")) {
            CreateAsset("New Prefab.bprefab", AssetType::Prefab);
        }
        
        ImGui::EndPopup();
    } else {
        m_showCreateMenu = false;
    }
}

void AssetBrowserPanel::RenderStatusBar() {
    ImGui::Separator();
    ImGui::Text("%zu items", m_currentAssets.size());
    
    if (!m_selectedAsset.empty()) {
        ImGui::SameLine();
        ImGui::Text(" | Selected: %s", fs::path(m_selectedAsset).filename().string().c_str());
    }
}

void AssetBrowserPanel::NavigateToDirectory(const std::string& path) {
    if (fs::exists(path) && fs::is_directory(path)) {
        m_currentDirectory = path;
        RefreshCurrentDirectory();
        m_selectedAsset.clear();
    }
}

void AssetBrowserPanel::RefreshCurrentDirectory() {
    ScanDirectory(m_currentDirectory);
}

void AssetBrowserPanel::ScanDirectory(const std::string& path) {
    m_currentAssets.clear();
    
    if (!fs::exists(path) || !fs::is_directory(path)) {
        return;
    }
    
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            AssetInfo asset;
            asset.path = entry.path().string();
            asset.name = entry.path().filename().string();
            asset.extension = entry.path().extension().string();
            asset.isDirectory = entry.is_directory();
            asset.type = GetAssetType(asset.path);
            
            try {
                asset.fileSize = entry.is_directory() ? 0 : entry.file_size();
                asset.lastModified = entry.last_write_time();
            } catch (const std::exception&) {
                asset.fileSize = 0;
                asset.lastModified = std::filesystem::file_time_type{};
            }
            
            m_currentAssets.push_back(asset);
        }
        
        // Sort: directories first, then by name
        std::sort(m_currentAssets.begin(), m_currentAssets.end(), 
                  [](const AssetInfo& a, const AssetInfo& b) {
                      if (a.isDirectory != b.isDirectory) {
                          return a.isDirectory;
                      }
                      return a.name < b.name;
                  });
        
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory " << path << ": " << e.what() << std::endl;
    }
}

AssetType AssetBrowserPanel::GetAssetType(const std::string& path) const {
    if (fs::is_directory(path)) {
        return AssetType::Folder;
    }
    
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
        return AssetType::Texture;
    } else if (ext == ".json") {
        // Determine if it's a material or scene by content/filename
        std::string filename = fs::path(path).stem().string();
        if (filename.find("material") != std::string::npos || filename.find("mat") != std::string::npos) {
            return AssetType::Material;
        } else if (filename.find("scene") != std::string::npos) {
            return AssetType::Scene;
        }
        return AssetType::Material; // Default for JSON files
    } else if (ext == ".bprefab") {
        return AssetType::Prefab;
    } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
        return AssetType::Audio;
    } else if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
        return AssetType::Script;
    } else if (ext == ".obj" || ext == ".fbx") {
        return AssetType::Model;
    } else if (ext == ".anim") {
        return AssetType::Animation;
    }
    
    return AssetType::Unknown;
}

std::string AssetBrowserPanel::GetAssetIconText(AssetType type) const {
    switch (type) {
        case AssetType::Folder:     return "F";
        case AssetType::Texture:    return "T";
        case AssetType::Material:   return "M";
        case AssetType::Scene:      return "S";
        case AssetType::Audio:      return "A";
        case AssetType::Script:     return "C";
        case AssetType::Prefab:     return "P";
        case AssetType::Model:      return "3";
        case AssetType::Animation:  return "~";
        default:                    return "?";
    }
}

bool AssetBrowserPanel::IsDirectoryExpanded(const std::string& path) const {
    auto it = m_expandedDirectories.find(path);
    return it != m_expandedDirectories.end() && it->second;
}

void AssetBrowserPanel::SetDirectoryExpanded(const std::string& path, bool expanded) {
    m_expandedDirectories[path] = expanded;
}

void AssetBrowserPanel::SelectAsset(const std::string& path) {
    m_selectedAsset = path;
    BroadcastSelectionChanged();
}

void AssetBrowserPanel::BroadcastSelectionChanged() {
    if (!m_eventBus) return;
    
    AssetType type = AssetType::Unknown;
    if (!m_selectedAsset.empty()) {
        type = GetAssetType(m_selectedAsset);
    }
    
    AssetSelectionChangedEvent event(m_selectedAsset, type);
    m_eventBus->Publish(event);
}

bool AssetBrowserPanel::CreateFolder(const std::string& name) {
    std::string folderPath = m_currentDirectory + "/" + name;
    
    // Find unique name if folder already exists
    int counter = 1;
    std::string uniquePath = folderPath;
    while (fs::exists(uniquePath)) {
        uniquePath = folderPath + " " + std::to_string(counter);
        counter++;
    }
    
    try {
        if (fs::create_directory(uniquePath)) {
            RefreshCurrentDirectory();
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating folder: " << e.what() << std::endl;
    }
    
    return false;
}

bool AssetBrowserPanel::CreateAsset(const std::string& name, AssetType type) {
    std::string assetPath = m_currentDirectory + "/" + name;
    
    // Find unique name if asset already exists
    fs::path basePath(assetPath);
    std::string stem = basePath.stem().string();
    std::string extension = basePath.extension().string();
    
    int counter = 1;
    std::string uniquePath = assetPath;
    while (fs::exists(uniquePath)) {
        uniquePath = m_currentDirectory + "/" + stem + " " + std::to_string(counter) + extension;
        counter++;
    }
    
    try {
        std::ofstream file(uniquePath);
        if (!file.is_open()) return false;
        
        // Write default content based on asset type
        switch (type) {
            case AssetType::Material:
                file << "{\n";
                file << "  \"name\": \"" << fs::path(uniquePath).stem().string() << "\",\n";
                file << "  \"type\": \"material\",\n";
                file << "  \"color\": [1.0, 1.0, 1.0, 1.0],\n";
                file << "  \"properties\": {\n";
                file << "    \"roughness\": 0.5,\n";
                file << "    \"metallic\": 0.0\n";
                file << "  }\n";
                file << "}\n";
                break;
                
            case AssetType::Scene:
                file << "{\n";
                file << "  \"name\": \"" << fs::path(uniquePath).stem().string() << "\",\n";
                file << "  \"type\": \"scene\",\n";
                file << "  \"entities\": [],\n";
                file << "  \"settings\": {\n";
                file << "    \"background\": [0.2, 0.3, 0.4, 1.0]\n";
                file << "  }\n";
                file << "}\n";
                break;
                
            case AssetType::Prefab:
                file << "{\n";
                file << "  \"name\": \"" << fs::path(uniquePath).stem().string() << "\",\n";
                file << "  \"type\": \"prefab\",\n";
                file << "  \"entity\": {\n";
                file << "    \"components\": []\n";
                file << "  }\n";
                file << "}\n";
                break;
                
            default:
                file << "// New asset\n";
                break;
        }
        
        file.close();
        RefreshCurrentDirectory();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating asset: " << e.what() << std::endl;
    }
    
    return false;
}

bool AssetBrowserPanel::DeleteAsset(const std::string& path) {
    try {
        if (fs::exists(path)) {
            if (fs::is_directory(path)) {
                fs::remove_all(path);
            } else {
                fs::remove(path);
            }
            RefreshCurrentDirectory();
            if (m_selectedAsset == path) {
                m_selectedAsset.clear();
            }
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error deleting asset: " << e.what() << std::endl;
    }
    
    return false;
}

bool AssetBrowserPanel::RenameAsset(const std::string& oldPath, const std::string& newName) {
    try {
        fs::path oldFsPath(oldPath);
        std::string extension = oldFsPath.extension().string();
        std::string newPath = oldFsPath.parent_path().string() + "/" + newName + extension;
        
        if (fs::exists(oldPath) && !fs::exists(newPath)) {
            fs::rename(oldPath, newPath);
            RefreshCurrentDirectory();
            if (m_selectedAsset == oldPath) {
                m_selectedAsset = newPath;
            }
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error renaming asset: " << e.what() << std::endl;
    }
    
    return false;
}

bool AssetBrowserPanel::DuplicateAsset(const std::string& path) {
    try {
        if (!fs::exists(path) || fs::is_directory(path)) {
            return false;
        }
        
        fs::path originalPath(path);
        std::string stem = originalPath.stem().string();
        std::string extension = originalPath.extension().string();
        std::string parentDir = originalPath.parent_path().string();
        
        // Find unique name
        int counter = 1;
        std::string newPath;
        do {
            newPath = parentDir + "/" + stem + " " + std::to_string(counter) + extension;
            counter++;
        } while (fs::exists(newPath));
        
        fs::copy_file(path, newPath);
        RefreshCurrentDirectory();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error duplicating asset: " << e.what() << std::endl;
    }
    
    return false;
}

bool AssetBrowserPanel::MoveAsset(const std::string& srcPath, const std::string& dstDirectory) {
    try {
        if (fs::exists(srcPath) && fs::exists(dstDirectory) && fs::is_directory(dstDirectory)) {
            std::string filename = fs::path(srcPath).filename().string();
            std::string dstPath = dstDirectory + "/" + filename;
            
            // Find unique name if destination exists
            int counter = 1;
            std::string uniqueDstPath = dstPath;
            while (fs::exists(uniqueDstPath)) {
                fs::path basePath(dstPath);
                std::string stem = basePath.stem().string();
                std::string extension = basePath.extension().string();
                uniqueDstPath = dstDirectory + "/" + stem + " " + std::to_string(counter) + extension;
                counter++;
            }
            
            fs::rename(srcPath, uniqueDstPath);
            RefreshCurrentDirectory();
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error moving asset: " << e.what() << std::endl;
    }
    
    return false;
}

void AssetBrowserPanel::HandleDragAndDrop() {
    // Drag and drop target for moving assets within the browser
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            std::string draggedAsset = static_cast<const char*>(payload->Data);
            MoveAsset(draggedAsset, m_currentDirectory);
        }
        ImGui::EndDragDropTarget();
    }
}

void AssetBrowserPanel::StartFileSystemWatcher() {
    // Simple polling-based file system monitoring
    // In a production system, you'd use platform-specific file system events
}

void AssetBrowserPanel::StopFileSystemWatcher() {
    // Cleanup file system watcher
}

void AssetBrowserPanel::CheckFileSystemChanges() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastRefresh).count();
    
    if (elapsed >= REFRESH_INTERVAL) {
        try {
            auto currentModTime = fs::last_write_time(m_currentDirectory);
            auto it = m_directoryModTimes.find(m_currentDirectory);
            
            if (it == m_directoryModTimes.end() || it->second != currentModTime) {
                RefreshCurrentDirectory();
                m_directoryModTimes[m_currentDirectory] = currentModTime;
            }
        } catch (const std::exception&) {
            // Ignore filesystem errors
        }
        
        m_lastRefresh = now;
    }
}

} // namespace BGE
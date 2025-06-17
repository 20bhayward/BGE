#include "AssetBrowserPanel.h"
#include "../../ServiceLocator.h"
#include "../../../Renderer/Renderer.h"
#include "../../Events.h"
#include "../../Services.h"
#include "ProjectSettingsPanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <cctype>
#include <ctime>

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
}

void AssetBrowserPanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoCollapse);
    
    // Get services
    m_eventBus = ServiceLocator::Instance().GetService<EventBus>().get();
    m_assetManager = ServiceLocator::Instance().GetService<AssetManager>().get();
    m_iconManager = &IconManager::Instance();
    
    RegisterEventListeners();
    RefreshCurrentDirectory();
}

void AssetBrowserPanel::RegisterEventListeners() {
    if (m_eventBus) {
        m_eventBus->Subscribe<AssetReloadedEvent>([this](const AssetReloadedEvent& event) {
            OnAssetReloaded(event);
        });
    }
}

void AssetBrowserPanel::UnregisterEventListeners() {
    // Event bus handles cleanup automatically in destructor
}

void AssetBrowserPanel::OnAssetReloaded(const AssetReloadedEvent& event) {
    // Refresh asset data for reloaded assets
    for (auto& asset : m_currentAssets) {
        if (asset.handle == event.handle) {
            // Asset data will be refreshed on next render
            break;
        }
    }
}

void AssetBrowserPanel::OnRender() {
    CheckFileSystemChanges();
    
    // Handle keyboard shortcuts
    HandleKeyboardShortcuts();
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    RenderToolbar();
    RenderBreadcrumbs();
    
    // Splitter
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    
    ImGui::BeginChild("LeftPanel", ImVec2(m_leftPanelWidth, 0), true);
    RenderLeftPanel();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Splitter bar - invisible button for resize
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    ImGui::Button("##splitter", ImVec2(4.0f, -1));
    ImGui::PopStyleColor(3);
    
    if (ImGui::IsItemActive()) {
        m_leftPanelWidth += ImGui::GetIO().MouseDelta.x;
        m_leftPanelWidth = (std::max)(150.0f, (std::min)(400.0f, m_leftPanelWidth));
    }
    
    // Show resize cursor when hovering
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    
    ImGui::SameLine();
    
    ImGui::BeginChild("MainPanel", ImVec2(0, -25), true);
    RenderMainPanel();
    ImGui::EndChild();
    
    // Status bar
    RenderStatusBar();
    
    ImGui::PopStyleVar();
    
    // Context menus
    if (m_showCreateMenu) {
        RenderCreateContextMenu();
    }
    
    ImGui::PopStyleVar();
}

void AssetBrowserPanel::RenderToolbar() {
    ImGui::Spacing();
    ImGui::Indent(8.0f);
    
    // Navigation buttons
    if (ImGui::Button("< Back")) {
        if (m_currentDirectory != m_assetsDirectory) {
            NavigateToDirectory(fs::path(m_currentDirectory).parent_path().string());
        }
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Refresh")) {
        RefreshCurrentDirectory();
    }
    ImGui::SameLine();
    
    // Creation buttons
    if (ImGui::Button("+ Folder")) {
        CreateFolder("New Folder");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Create New Folder");
    }
    ImGui::SameLine();
    
    if (ImGui::Button("+ Asset")) {
        ImGui::OpenPopup("CreateAssetPopup");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Create New Asset");
    }
    ImGui::SameLine();
    
    // Create Asset Popup
    if (ImGui::BeginPopup("CreateAssetPopup")) {
        if (ImGui::MenuItem("Material")) {
            CreateAsset("New Material.json", AssetType::Material);
        }
        if (ImGui::MenuItem("Scene")) {
            CreateAsset("New Scene.json", AssetType::Scene);
        }
        if (ImGui::MenuItem("Text File")) {
            CreateAsset("New File.txt", AssetType::Unknown);
        }
        ImGui::EndPopup();
    }
    
    // Search bar
    ImGui::PushItemWidth(200);
    bool searchChanged = ImGui::InputText("##Search", m_searchBuffer, sizeof(m_searchBuffer));
    ImGui::PopItemWidth();
    
    if (searchChanged) {
        RefreshAssets();
    }
    
    ImGui::SameLine();
    
    // Filter dropdown
    const char* filterNames[] = { "All", "Textures", "Materials", "Prefabs", "Scenes", "Scripts" };
    AssetType filterTypes[] = { AssetType::Unknown, AssetType::Texture, AssetType::Material, 
                               AssetType::Prefab, AssetType::Scene, AssetType::Script };
    
    int currentFilter = 0;
    for (int i = 0; i < 6; ++i) {
        if (filterTypes[i] == m_filterType) {
            currentFilter = i;
            break;
        }
    }
    
    ImGui::PushItemWidth(100);
    if (ImGui::Combo("##Filter", &currentFilter, filterNames, 6)) {
        m_filterType = filterTypes[currentFilter];
        RefreshAssets();
    }
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    
    // Icon size slider
    ImGui::Text("Size:");
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    if (ImGui::SliderFloat("##IconSize", &m_iconSize, 32.0f, 128.0f, "%.0f")) {
        m_iconSize = (std::max)(32.0f, (std::min)(128.0f, m_iconSize));
    }
    ImGui::PopItemWidth();
    
    ImGui::Unindent(8.0f);
    ImGui::Separator();
}

void AssetBrowserPanel::RenderBreadcrumbs() {
    ImGui::Spacing();
    ImGui::Indent(8.0f);
    
    fs::path currentPath(m_currentDirectory);
    fs::path assetsPath(m_assetsDirectory);
    
    std::vector<std::string> pathParts;
    fs::path relativePath = fs::relative(currentPath, assetsPath);
    
    pathParts.push_back("Assets");
    
    if (relativePath != ".") {
        for (const auto& part : relativePath) {
            pathParts.push_back(part.string());
        }
    }
    
    for (size_t i = 0; i < pathParts.size(); ++i) {
        if (i > 0) {
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
        }
        
        if (ImGui::Button(pathParts[i].c_str())) {
            std::string newPath = m_assetsDirectory;
            for (size_t j = 1; j <= i; ++j) {
                newPath += "/" + pathParts[j];
            }
            NavigateToDirectory(newPath);
        }
    }
    
    ImGui::Unindent(8.0f);
    ImGui::Separator();
}

void AssetBrowserPanel::RenderLeftPanel() {
    ImGui::Text("Folders");
    ImGui::Separator();
    
    RenderDirectoryTree(m_assetsDirectory);
}

void AssetBrowserPanel::RenderDirectoryTree(const std::string& path, int depth) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        return;
    }
    
    std::string folderName = fs::path(path).filename().string();
    if (folderName.empty()) {
        folderName = "Assets";
    }
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    
    if (path == m_currentDirectory) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    bool hasSubdirs = false;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                hasSubdirs = true;
                break;
            }
        }
    } catch (...) {
        // Ignore errors
    }
    
    if (!hasSubdirs) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    
    bool nodeOpen = ImGui::TreeNodeEx(folderName.c_str(), flags);
    
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        NavigateToDirectory(path);
    }
    
    if (nodeOpen && hasSubdirs) {
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.is_directory()) {
                    RenderDirectoryTree(entry.path().string(), depth + 1);
                }
            }
        } catch (...) {
            // Ignore errors
        }
        
        if (hasSubdirs) {
            ImGui::TreePop();
        }
    }
}

void AssetBrowserPanel::RenderMainPanel() {
    // Background context menu for creating new assets
    if (ImGui::BeginPopupContextWindow("AssetBrowserContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        ImGui::Text("Create New");
        ImGui::Separator();
        
        if (ImGui::MenuItem("Folder")) {
            CreateFolder("New Folder");
        }
        
        if (ImGui::BeginMenu("Asset")) {
            if (ImGui::MenuItem("Material")) {
                CreateAsset("New Material.json", AssetType::Material);
            }
            if (ImGui::MenuItem("Scene")) {
                CreateAsset("New Scene.json", AssetType::Scene);
            }
            if (ImGui::MenuItem("Text File")) {
                CreateAsset("New File.txt", AssetType::Unknown);
            }
            ImGui::EndMenu();
        }
        
        ImGui::Separator();
        
        // Paste option
        if (CanPaste()) {
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                PasteAssets(m_currentDirectory);
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::MenuItem("Paste", "Ctrl+V");
            ImGui::EndDisabled();
        }
        
        ImGui::EndPopup();
    }
    
    RenderAssetGrid();
    
    HandleDragAndDrop();
}

void AssetBrowserPanel::RenderAssetGrid() {
    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    
    // Calculate grid dimensions with padding
    float iconSpacing = 8.0f;
    float iconWithSpacing = m_iconSize + iconSpacing * 2; // Extra padding
    int maxColumns = (std::max)(1, (int)((panelSize.x - 20.0f) / iconWithSpacing)); // Account for scrollbar
    
    // Use a table for better control over layout
    if (ImGui::BeginTable("AssetGrid", maxColumns, ImGuiTableFlags_NoHostExtendX)) {
        
        int currentColumn = 0;
        for (const auto& asset : m_currentAssets) {
            // Filter out .meta files unless explicitly shown
            if (!m_showMetaFiles && asset.extension == ".meta") {
                continue;
            }
            
            // Apply search filter
            if (strlen(m_searchBuffer) > 0) {
                std::string searchTerm = m_searchBuffer;
                std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);
                
                std::string assetName = asset.name;
                std::transform(assetName.begin(), assetName.end(), assetName.begin(), ::tolower);
                
                if (assetName.find(searchTerm) == std::string::npos) {
                    continue;
                }
            }
            
            // Apply type filter
            if (m_filterType != AssetType::Unknown && asset.type != m_filterType) {
                continue;
            }
            
            // Start new row if needed
            if (currentColumn == 0) {
                ImGui::TableNextRow();
            }
            
            ImGui::TableSetColumnIndex(currentColumn);
            
            // Center the icon in the cell
            ImVec2 cellSize = ImVec2(iconWithSpacing, m_iconSize + 20.0f); // Extra height for text
            ImVec2 cursorPos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(cursorPos.x + iconSpacing/2, cursorPos.y + iconSpacing/2));
            
            RenderAssetIcon(asset, ImVec2(m_iconSize, m_iconSize));
            
            
            currentColumn = (currentColumn + 1) % maxColumns;
        }
        
        ImGui::EndTable();
    }
}

void AssetBrowserPanel::RenderAssetIcon(const AssetInfo& asset, const ImVec2& iconSize) {
    ImGui::BeginGroup();
    
    bool clicked = false;
    
    // Always create a button area of consistent size first
    ImVec2 cursorStart = ImGui::GetCursorPos();
    
    // Check for individual asset thumbnail first, then type-based thumbnails
    ProjectSettingsPanel* projectSettings = Services::GetProjectSettings();
    uint32_t customThumbnailId = 0;
    if (projectSettings) {
        // First check for individual asset thumbnail using asset handle
        if (asset.handle.IsValid()) {
            customThumbnailId = projectSettings->GetIndividualAssetThumbnail(asset.handle);
        }
        
        // If no thumbnail found by handle, try path-based lookup (for moved/recreated assets)
        if (customThumbnailId == 0) {
            customThumbnailId = projectSettings->GetIndividualAssetThumbnailByPath(asset.path);
            
            // If found by path but not by handle, restore the mapping
            if (customThumbnailId != 0 && asset.handle.IsValid()) {
                projectSettings->RestoreThumbnailFromPath(asset.handle, asset.path);
            }
        }
        
        // If no individual thumbnail, check for type-based thumbnail
        if (customThumbnailId == 0) {
            customThumbnailId = projectSettings->GetAssetTypeThumbnailTexture(asset.type);
        }
    }
    
    if (customThumbnailId != 0) {
        // Use custom thumbnail image with padding to match button style
        std::string buttonId = "##thumbnail_" + asset.path;
        
        // Apply button padding to match regular button size
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding);
        clicked = ImGui::ImageButton(
            buttonId.c_str(),
            static_cast<ImTextureID>(static_cast<uintptr_t>(customThumbnailId)), 
            ImVec2(iconSize.x - ImGui::GetStyle().FramePadding.x * 2, iconSize.y - ImGui::GetStyle().FramePadding.y * 2)
        );
        ImGui::PopStyleVar();
    } else {
        // Fallback to FontAwesome icons with consistent size
        std::string iconText = m_iconManager ? m_iconManager->GetIconText(asset.type) : "FILE";
        
        // Style the button based on asset type
        ImVec4 iconColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        switch (asset.type) {
            case AssetType::Texture: iconColor = ImVec4(0.2f, 0.7f, 0.2f, 1.0f); break;
            case AssetType::Material: iconColor = ImVec4(0.7f, 0.4f, 0.2f, 1.0f); break;
            case AssetType::Scene: iconColor = ImVec4(0.2f, 0.4f, 0.7f, 1.0f); break;
            case AssetType::Audio: iconColor = ImVec4(0.7f, 0.2f, 0.7f, 1.0f); break;
            case AssetType::Script: iconColor = ImVec4(0.7f, 0.7f, 0.2f, 1.0f); break;
            case AssetType::Prefab: iconColor = ImVec4(0.2f, 0.7f, 0.7f, 1.0f); break;
            case AssetType::Folder: iconColor = ImVec4(0.9f, 0.8f, 0.4f, 1.0f); break;
            default: iconColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
        }
        
        ImGui::PushStyleColor(ImGuiCol_Button, iconColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(iconColor.x * 1.2f, iconColor.y * 1.2f, iconColor.z * 1.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(iconColor.x * 1.4f, iconColor.y * 1.4f, iconColor.z * 1.4f, 1.0f));
        
        // Create button with exact size
        clicked = ImGui::Button(iconText.c_str(), iconSize);
        ImGui::PopStyleColor(3);
    }
    
    // Selection highlight for multi-selection
    bool isSelected = (asset.path == m_selectedAsset) || 
                     (std::find(m_multiSelection.begin(), m_multiSelection.end(), asset.path) != m_multiSelection.end());
    
    if (isSelected) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        
        // Primary selection gets different color
        if (asset.path == m_selectedAsset) {
            drawList->AddRect(min, max, IM_COL32(100, 150, 255, 180), 2.0f, 0, 2.0f); // Blue for primary
        } else {
            drawList->AddRect(min, max, IM_COL32(100, 150, 255, 100), 2.0f, 0, 1.0f); // Lighter blue for multi-selection
        }
    }
    
    // Handle clicks - single click only selects, doesn't navigate
    if (clicked) {
        ImGuiIO& io = ImGui::GetIO();
        
        if (io.KeyCtrl) {
            // Ctrl+Click: Toggle selection
            auto it = std::find(m_multiSelection.begin(), m_multiSelection.end(), asset.path);
            if (it != m_multiSelection.end()) {
                m_multiSelection.erase(it);
                if (m_selectedAsset == asset.path) {
                    m_selectedAsset = m_multiSelection.empty() ? "" : m_multiSelection.back();
                }
            } else {
                m_multiSelection.push_back(asset.path);
                m_selectedAsset = asset.path;
            }
        } else if (io.KeyShift && !m_selectedAsset.empty()) {
            // Shift+Click: Range selection (implement basic version)
            m_multiSelection.clear();
            bool foundStart = false;
            bool foundEnd = false;
            
            for (const auto& currentAsset : m_currentAssets) {
                if (currentAsset.path == m_selectedAsset || currentAsset.path == asset.path) {
                    if (!foundStart) {
                        foundStart = true;
                    } else if (!foundEnd) {
                        foundEnd = true;
                    }
                }
                
                if (foundStart) {
                    m_multiSelection.push_back(currentAsset.path);
                }
                
                if (foundEnd) {
                    break;
                }
            }
            
            m_selectedAsset = asset.path;
        } else {
            // Regular click: Clear selection and select this asset
            m_multiSelection.clear();
            SelectAsset(asset.path);
        }
        
        BroadcastSelectionChanged();
    }
    
    // Double-click handling for folders and files
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        if (asset.isDirectory) {
            NavigateToDirectory(asset.path);
        }
        // For files, double-click could open in external editor or load asset
    }
    
    // Context menu - support for multi-selection
    if (ImGui::BeginPopupContextItem(("AssetMenu##" + asset.path).c_str())) {
        // If this asset isn't in the multi-selection, select only this asset
        if (std::find(m_multiSelection.begin(), m_multiSelection.end(), asset.path) == m_multiSelection.end()) {
            m_multiSelection.clear();
            m_multiSelection.push_back(asset.path);
            m_selectedAsset = asset.path;
        }
        
        m_contextMenuAsset = asset.path;
        m_selectedAssetsForMenu = m_multiSelection.empty() ? std::vector<std::string>{asset.path} : m_multiSelection;
        
        RenderAssetMenuContent();
        
        ImGui::EndPopup();
    }
    
    // Handle drag and drop initiation
    BeginDragAsset(asset);
    
    // Handle drop target for folders
    if (asset.isDirectory && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ITEM")) {
            std::string droppedAssetPath((const char*)payload->Data);
            std::cout << "Drop onto folder: " << asset.name << " from " << droppedAssetPath << std::endl;
            
            // Don't allow dropping into itself or its children
            fs::path draggedPath(droppedAssetPath);
            fs::path targetDir(asset.path);
            
            if (draggedPath != targetDir && !fs::equivalent(draggedPath.parent_path(), targetDir)) {
                if (MoveAsset(droppedAssetPath, asset.path)) {
                    std::cout << "Successfully moved asset into folder: " << asset.path << std::endl;
                    RefreshAssets();
                } else {
                    std::cout << "Failed to move asset into folder" << std::endl;
                }
            } else {
                std::cout << "Cannot drop folder into itself or same directory" << std::endl;
            }
        }
        ImGui::EndDragDropTarget();
    }
    
    // Asset name (with rename support)
    if (m_renameMode && asset.path == m_renamingAsset) {
        ImGui::SetNextItemWidth(iconSize.x);
        
        bool enterPressed = ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), 
                                           ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        bool inputActive = ImGui::IsItemActive();
        bool escapePressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
        
        // Store previous active state to detect focus loss
        bool justLostFocus = m_renameInputWasActive && !inputActive;
        m_renameInputWasActive = inputActive;
        
        // Auto-save on Enter or when losing focus (but not on Escape)
        if (enterPressed || (justLostFocus && !escapePressed && strlen(m_renameBuffer) > 0)) {
            if (strlen(m_renameBuffer) > 0 && strcmp(m_renameBuffer, std::filesystem::path(asset.path).stem().string().c_str()) != 0) {
                // For files, need to preserve the extension
                std::string newName = m_renameBuffer;
                if (!asset.isDirectory) {
                    std::string extension = std::filesystem::path(asset.path).extension().string();
                    if (!extension.empty()) {
                        newName += extension;
                    }
                }
                
                std::string newPath = std::filesystem::path(asset.path).parent_path().string() + "/" + newName;
                if (RenameAsset(asset.path, newName)) {
                    // Update selected asset to new path and broadcast to update Inspector
                    m_selectedAsset = newPath;
                    BroadcastSelectionChanged();
                }
            }
            m_renameMode = false;
            m_renamingAsset.clear();
            m_renameInputWasActive = false;
        }
        // Cancel on Escape
        else if (escapePressed) {
            m_renameMode = false;
            m_renamingAsset.clear();
            m_renameInputWasActive = false;
        }
    } else {
        // Center the text below the icon
        std::string displayName = asset.name;
        
        // Calculate text size to center it
        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        float textWidth = (std::min)(textSize.x, iconSize.x);
        
        // Center horizontally
        ImVec2 currentPos = ImGui::GetCursorPos();
        float centerOffset = (iconSize.x - textWidth) * 0.5f;
        if (centerOffset > 0) {
            ImGui::SetCursorPosX(currentPos.x + centerOffset);
        }
        
        // Truncate text if too long and render
        if (textSize.x > iconSize.x) {
            // Truncate with ellipsis
            size_t maxChars = (size_t)(displayName.length() * iconSize.x / textSize.x) - 3;
            if (maxChars > 0 && maxChars < displayName.length()) {
                displayName = displayName.substr(0, maxChars) + "...";
            }
        }
        
        ImGui::Text("%s", displayName.c_str());
        
        // Check for double-click to start renaming (only on text, not icon)
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            m_renameMode = true;
            m_renamingAsset = asset.path;
            m_renameInputWasActive = false; // Initialize state
            std::string name = std::filesystem::path(asset.path).stem().string();
            strncpy(m_renameBuffer, name.c_str(), sizeof(m_renameBuffer) - 1);
            m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
        }
    }
    
    ImGui::EndGroup();
}

void AssetBrowserPanel::RenderStatusBar() {
    ImGui::Separator();
    
    // Count visible assets (excluding .meta files if hidden)
    int visibleAssetCount = 0;
    for (const auto& asset : m_currentAssets) {
        if (!m_showMetaFiles && asset.extension == ".meta") {
            continue;
        }
        visibleAssetCount++;
    }
    
    // Selection info
    int selectionCount = static_cast<int>(m_multiSelection.size());
    if (!m_selectedAsset.empty() && selectionCount == 0) {
        selectionCount = 1; // Single selection not in multi-selection array
    }
    
    // Status text
    std::string statusText = std::to_string(visibleAssetCount) + " items";
    if (selectionCount > 0) {
        statusText += " (" + std::to_string(selectionCount) + " selected)";
    }
    
    // Current directory info
    std::string currentDirName = fs::path(m_currentDirectory).filename().string();
    if (currentDirName.empty()) {
        currentDirName = "Assets";
    }
    
    ImGui::Text("%s", statusText.c_str());
    ImGui::SameLine();
    
    // Right-aligned current directory
    std::string dirText = "Location: " + currentDirName;
    float textWidth = ImGui::CalcTextSize(dirText.c_str()).x;
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX(windowWidth - textWidth - 10.0f);
    ImGui::Text("%s", dirText.c_str());
}

// void AssetBrowserPanel::RenderStatusBar() {
//     ImGui::Separator();
//     ImGui::Text("Assets: %zu", m_currentAssets.size());
//     
//     if (!m_selectedAsset.empty()) {
//         ImGui::SameLine();
//         ImGui::Text("| Selected: %s", fs::path(m_selectedAsset).filename().string().c_str());
//     }
// }

void AssetBrowserPanel::RenderCreateContextMenu() {
    if (ImGui::BeginPopupContextWindow("CreateAssetMenu")) {
        if (ImGui::MenuItem("Create Folder")) {
            CreateFolder("New Folder");
            m_showCreateMenu = false;
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Create Material")) {
            CreateAsset("New Material.json", AssetType::Material);
            m_showCreateMenu = false;
        }
        
        if (ImGui::MenuItem("Create Scene")) {
            CreateAsset("New Scene.json", AssetType::Scene);
            m_showCreateMenu = false;
        }
        
        ImGui::EndPopup();
    } else {
        m_showCreateMenu = false;
    }
}

void AssetBrowserPanel::RenderAssetContextMenu() {
    // This method is no longer used as we use BeginPopupContextItem directly
}

void AssetBrowserPanel::RenderAssetMenuContent() {
    bool isMultiSelection = m_selectedAssetsForMenu.size() > 1;
    std::string selectionText = isMultiSelection ? 
        std::to_string(m_selectedAssetsForMenu.size()) + " items" : 
        fs::path(m_contextMenuAsset).filename().string();
    
    ImGui::Text("Selected: %s", selectionText.c_str());
    ImGui::Separator();
    
    // Rename (only for single selection)
    if (!isMultiSelection && ImGui::MenuItem("Rename", "F2")) {
        std::string assetPath = m_selectedAssetsForMenu[0];
        m_renameMode = true;
        m_renamingAsset = assetPath;
        m_renameInputWasActive = false;
        std::string name = fs::path(assetPath).stem().string();
        strncpy(m_renameBuffer, name.c_str(), sizeof(m_renameBuffer) - 1);
        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
    }
    
    ImGui::Separator();
    
    // Copy
    if (ImGui::MenuItem("Copy", "Ctrl+C")) {
        CopyAssets(m_selectedAssetsForMenu);
    }
    
    // Cut
    if (ImGui::MenuItem("Cut", "Ctrl+X")) {
        CutAssets(m_selectedAssetsForMenu);
    }
    
    // Paste (if clipboard has content)
    if (CanPaste()) {
        if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            PasteAssets(m_currentDirectory);
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::MenuItem("Paste", "Ctrl+V");
        ImGui::EndDisabled();
    }
    
    ImGui::Separator();
    
    // Duplicate (only for single selection)
    if (!isMultiSelection && ImGui::MenuItem("Duplicate", "Ctrl+D")) {
        DuplicateAsset(m_selectedAssetsForMenu[0]);
    }
    
    ImGui::Separator();
    
    // Delete
    if (ImGui::MenuItem("Delete", "Del")) {
        if (isMultiSelection) {
            DeleteAssets(m_selectedAssetsForMenu);
        } else {
            DeleteAsset(m_selectedAssetsForMenu[0]);
        }
    }
    
    ImGui::Separator();
    
    // Properties (future implementation)
    ImGui::BeginDisabled();
    ImGui::MenuItem("Properties", "Alt+Enter");
    ImGui::EndDisabled();
}

void AssetBrowserPanel::NavigateToDirectory(const std::string& path) {
    if (fs::exists(path) && fs::is_directory(path)) {
        m_currentDirectory = path;
        RefreshCurrentDirectory();
    }
}

void AssetBrowserPanel::RefreshCurrentDirectory() {
    ScanDirectory(m_currentDirectory);
}

void AssetBrowserPanel::ScanDirectory(const std::string& path) {
    m_currentAssets.clear();
    
    if (!fs::exists(path)) {
        return;
    }
    
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            AssetInfo assetInfo;
            assetInfo.path = entry.path().string();
            assetInfo.name = entry.path().filename().string();
            assetInfo.isDirectory = entry.is_directory();
            assetInfo.lastModified = entry.last_write_time();
            
            if (!assetInfo.isDirectory) {
                assetInfo.extension = entry.path().extension().string();
                assetInfo.type = GetAssetType(assetInfo.path);
                assetInfo.fileSize = entry.file_size();
                
                // Get asset handle from registry
                if (m_assetManager) {
                    assetInfo.handle = m_assetManager->GetRegistry().GetAssetHandle(assetInfo.path);
                }
            } else {
                assetInfo.type = AssetType::Folder;
            }
            
            m_currentAssets.push_back(assetInfo);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory " << path << ": " << e.what() << std::endl;
    }
    
    // Sort assets (directories first)
    std::sort(m_currentAssets.begin(), m_currentAssets.end(), 
        [](const AssetInfo& a, const AssetInfo& b) {
            if (a.isDirectory != b.isDirectory) {
                return a.isDirectory;
            }
            return a.name < b.name;
        });
}

AssetType AssetBrowserPanel::GetAssetType(const std::string& path) const {
    if (m_assetManager) {
        return m_assetManager->GetRegistry().GetAssetType(
            m_assetManager->GetRegistry().GetAssetHandle(path));
    }
    
    // Fallback to extension-based detection
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
        return AssetType::Texture;
    } else if (ext == ".json") {
        std::string filename = fs::path(path).filename().string();
        std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
        
        if (filename.find("material") != std::string::npos) {
            return AssetType::Material;
        } else if (filename.find("scene") != std::string::npos) {
            return AssetType::Scene;
        }
    } else if (ext == ".bprefab") {
        return AssetType::Prefab;
    }
    
    return AssetType::Unknown;
}

uint32_t AssetBrowserPanel::GetAssetIcon(AssetType type) const {
    return m_iconManager ? m_iconManager->GetIcon(type) : 0;
}

void AssetBrowserPanel::SelectAsset(const std::string& path) {
    m_selectedAsset = path;
    BroadcastSelectionChanged();
}

void AssetBrowserPanel::BroadcastSelectionChanged() {
    if (m_eventBus && !m_selectedAsset.empty()) {
        AssetType type = GetAssetType(m_selectedAsset);
        m_eventBus->Publish(AssetSelectionChangedEvent(m_selectedAsset, type));
    }
}

void AssetBrowserPanel::CheckFileSystemChanges() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastRefresh).count();
    
    if (elapsed >= REFRESH_INTERVAL) {
        RefreshAssets();
        m_lastRefresh = now;
    }
}

void AssetBrowserPanel::HandleKeyboardShortcuts() {
    // Only handle shortcuts when this panel has focus
    if (!ImGui::IsWindowFocused()) {
        return;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    
    // Copy (Ctrl+C)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C) && !m_selectedAsset.empty()) {
        std::vector<std::string> selection = m_multiSelection.empty() ? 
            std::vector<std::string>{m_selectedAsset} : m_multiSelection;
        CopyAssets(selection);
    }
    
    // Cut (Ctrl+X)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X) && !m_selectedAsset.empty()) {
        std::vector<std::string> selection = m_multiSelection.empty() ? 
            std::vector<std::string>{m_selectedAsset} : m_multiSelection;
        CutAssets(selection);
    }
    
    // Paste (Ctrl+V)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V) && CanPaste()) {
        PasteAssets(m_currentDirectory);
    }
    
    // Duplicate (Ctrl+D)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D) && !m_selectedAsset.empty() && m_multiSelection.size() <= 1) {
        DuplicateAsset(m_selectedAsset);
    }
    
    // Delete (Del)
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !m_selectedAsset.empty()) {
        std::vector<std::string> selection = m_multiSelection.empty() ? 
            std::vector<std::string>{m_selectedAsset} : m_multiSelection;
        
        if (selection.size() > 1) {
            DeleteAssets(selection);
        } else {
            DeleteAsset(selection[0]);
        }
    }
    
    // Rename (F2)
    if (ImGui::IsKeyPressed(ImGuiKey_F2) && !m_selectedAsset.empty() && m_multiSelection.size() <= 1) {
        m_renameMode = true;
        m_renamingAsset = m_selectedAsset;
        m_renameInputWasActive = false;
        std::string name = fs::path(m_selectedAsset).stem().string();
        strncpy(m_renameBuffer, name.c_str(), sizeof(m_renameBuffer) - 1);
        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
    }
    
    // Refresh (F5)
    if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
        RefreshCurrentDirectory();
    }
    
    // Select All (Ctrl+A)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
        m_multiSelection.clear();
        for (const auto& asset : m_currentAssets) {
            // Filter out .meta files from selection
            if (!m_showMetaFiles && asset.extension == ".meta") {
                continue;
            }
            m_multiSelection.push_back(asset.path);
        }
        if (!m_multiSelection.empty()) {
            m_selectedAsset = m_multiSelection[0];
            BroadcastSelectionChanged();
        }
    }
}

void AssetBrowserPanel::RefreshAssets() {
    ScanDirectory(m_currentDirectory);
}

bool AssetBrowserPanel::CreateFolder(const std::string& name) {
    if (!ValidateAssetName(name, true)) {
        std::cerr << "Invalid folder name: " << name << std::endl;
        return false;
    }
    
    // Generate globally unique name
    std::string uniqueName = GenerateUniqueAssetNameGlobally(name);
    std::string folderPath = m_currentDirectory + "/" + uniqueName;
    
    try {
        fs::create_directory(folderPath);
        
        // Notify asset system
        NotifyAssetSystemOfChanges(folderPath, "created");
        
        RefreshCurrentDirectory();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating folder: " << e.what() << std::endl;
        return false;
    }
}

bool AssetBrowserPanel::CreateAsset(const std::string& name, AssetType type) {
    if (!ValidateAssetName(name, false)) {
        std::cerr << "Invalid asset name: " << name << std::endl;
        return false;
    }
    
    // Generate globally unique name
    std::string uniqueName = GenerateUniqueAssetNameGlobally(name);
    std::string assetPath = m_currentDirectory + "/" + uniqueName;
    
    try {
        std::ofstream file(assetPath);
        
        if (type == AssetType::Material) {
            file << "{\n"
                 << "  \"color\": [1.0, 1.0, 1.0, 1.0],\n"
                 << "  \"roughness\": 0.5,\n"
                 << "  \"metallic\": 0.0,\n"
                 << "  \"emission\": 0.0\n"
                 << "}";
        } else if (type == AssetType::Scene) {
            file << "{\n"
                 << "  \"entities\": [],\n"
                 << "  \"metadata\": {\n"
                 << "    \"version\": 1\n"
                 << "  }\n"
                 << "}";
        } else if (type == AssetType::Unknown) {
            // Create empty text file or basic content based on extension
            std::string ext = fs::path(name).extension().string();
            if (ext == ".txt") {
                file << "// New text file created in BGE Asset Browser\n";
            } else {
                file << "";
            }
        }
        
        file.close();
        
        // Register with asset manager
        if (m_assetManager) {
            m_assetManager->LoadAsset(assetPath);
        }
        
        // Notify asset system
        NotifyAssetSystemOfChanges(assetPath, "created");
        
        RefreshCurrentDirectory();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating asset: " << e.what() << std::endl;
        return false;
    }
}

bool AssetBrowserPanel::DeleteAsset(const std::string& path) {
    try {
        AssetHandle handle;
        if (m_assetManager) {
            handle = m_assetManager->GetRegistry().GetAssetHandle(path);
            if (handle.IsValid()) {
                m_assetManager->UnloadAsset(handle);
            }
        }
        
        // Clean up thumbnail mappings before deleting the asset
        auto projectSettings = Services::GetProjectSettings();
        if (projectSettings) {
            if (handle.IsValid()) {
                projectSettings->RemoveIndividualAssetThumbnail(handle);
            }
            // Also remove by path in case the handle lookup failed
            projectSettings->RemoveIndividualAssetThumbnailByPath(path);
            projectSettings->SaveProjectSettings();
        }
        
        fs::remove_all(path);
        
        // Notify asset system
        NotifyAssetSystemOfChanges(path, "deleted");
        
        RefreshCurrentDirectory();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting asset: " << e.what() << std::endl;
        return false;
    }
}

bool AssetBrowserPanel::RenameAsset(const std::string& oldPath, const std::string& newName) {
    try {
        std::string newPath = fs::path(oldPath).parent_path().string() + "/" + newName;
        
        // Get old asset handle before renaming
        AssetHandle oldHandle;
        if (m_assetManager) {
            oldHandle = m_assetManager->GetRegistry().GetAssetHandle(oldPath);
        }
        
        fs::rename(oldPath, newPath);
        
        if (m_assetManager) {
            m_assetManager->GetRegistry().RefreshAsset(newPath);
            
            // Get new asset handle after renaming
            AssetHandle newHandle = m_assetManager->GetRegistry().GetAssetHandle(newPath);
            
            // Update thumbnail mappings if we have both handles
            if (oldHandle.IsValid() && newHandle.IsValid()) {
                auto projectSettings = Services::GetProjectSettings();
                if (projectSettings) {
                    projectSettings->UpdateAssetThumbnailMapping(oldHandle, newHandle, newPath);
                    projectSettings->SaveProjectSettings();
                }
            }
        }
        
        RefreshCurrentDirectory();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error renaming asset: " << e.what() << std::endl;
        return false;
    }
}

bool AssetBrowserPanel::DuplicateAsset(const std::string& path) {
    try {
        fs::path sourcePath(path);
        std::string baseName = sourcePath.stem().string();
        std::string extension = sourcePath.extension().string();
        std::string copyBaseName = baseName + " Copy" + extension;
        
        // Generate unique name
        std::string uniqueName = GenerateUniqueAssetName(copyBaseName, sourcePath.parent_path().string());
        std::string newPath = sourcePath.parent_path().string() + "/" + uniqueName;
        
        if (fs::is_directory(path)) {
            fs::copy(path, newPath, fs::copy_options::recursive);
        } else {
            fs::copy_file(path, newPath);
        }
        
        if (m_assetManager) {
            m_assetManager->LoadAsset(newPath);
        }
        
        // Notify asset system
        NotifyAssetSystemOfChanges(newPath, "duplicated");
        
        RefreshCurrentDirectory();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error duplicating asset: " << e.what() << std::endl;
        return false;
    }
}


void AssetBrowserPanel::HandleDragAndDrop() {
    // Reset drag state when drag ends
    if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        if (m_isDragging) {
            std::cout << "Drag ended" << std::endl;
        }
        m_isDragging = false;
        m_draggedAsset.clear();
    }
}

void AssetBrowserPanel::BeginDragAsset(const AssetInfo& asset) {
    // Check if the item (icon/button) was clicked and can be dragged
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        // Set the payload data
        ImGui::SetDragDropPayload("ASSET_ITEM", asset.path.c_str(), asset.path.size() + 1);
        
        // Minimal visual feedback during drag (no yellow highlighting)
        ImGui::Text("%s", asset.name.c_str());
        
        // Update drag state
        if (!m_isDragging) {
            m_isDragging = true;
            m_draggedAsset = asset.path;
            m_draggedAssetInfo = asset;
            std::cout << "Started dragging: " << asset.name << std::endl;
        }
        
        ImGui::EndDragDropSource();
        std::cout << "Drag drop source active for: " << asset.name << std::endl;
    }
}

bool AssetBrowserPanel::IsDirectoryExpanded(const std::string& path) const {
    auto it = m_expandedDirectories.find(path);
    return it != m_expandedDirectories.end() ? it->second : false;
}

void AssetBrowserPanel::SetDirectoryExpanded(const std::string& path, bool expanded) {
    m_expandedDirectories[path] = expanded;
}

bool AssetBrowserPanel::MoveAsset(const std::string& srcPath, const std::string& dstDirectory) {
    try {
        std::string fileName = fs::path(srcPath).filename().string();
        std::string newPath = dstDirectory + "/" + fileName;
        
        // Get old asset handle before moving
        AssetHandle oldHandle;
        if (m_assetManager) {
            oldHandle = m_assetManager->GetRegistry().GetAssetHandle(srcPath);
        }
        
        fs::rename(srcPath, newPath);
        
        if (m_assetManager) {
            m_assetManager->GetRegistry().RefreshAsset(newPath);
            
            // Get new asset handle after moving
            AssetHandle newHandle = m_assetManager->GetRegistry().GetAssetHandle(newPath);
            
            // Update thumbnail mappings if we have both handles
            if (oldHandle.IsValid() && newHandle.IsValid()) {
                auto projectSettings = Services::GetProjectSettings();
                if (projectSettings) {
                    projectSettings->UpdateAssetThumbnailMapping(oldHandle, newHandle, newPath);
                    projectSettings->SaveProjectSettings();
                }
            }
        }
        
        RefreshCurrentDirectory();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error moving asset: " << e.what() << std::endl;
        return false;
    }
}

// Professional Asset Management Implementation

std::string AssetBrowserPanel::GenerateUniqueAssetNameGlobally(const std::string& baseName) {
    // Check if name exists anywhere in the project
    fs::path baseNamePath(baseName);
    std::string stem = baseNamePath.stem().string();
    std::string extension = baseNamePath.extension().string();
    
    std::string candidateName = baseName;
    int counter = 1;
    
    // Check entire assets directory recursively
    while (true) {
        bool nameExists = false;
        
        try {
            for (const auto& entry : fs::recursive_directory_iterator(m_assetsDirectory)) {
                if (entry.path().filename().string() == candidateName) {
                    nameExists = true;
                    break;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error checking for duplicate names: " << e.what() << std::endl;
            break;
        }
        
        if (!nameExists) {
            break;
        }
        
        // Generate next candidate name
        candidateName = stem + " " + std::to_string(counter) + extension;
        counter++;
        
        // Safety limit to prevent infinite loops
        if (counter > 9999) {
            candidateName = stem + "_" + std::to_string(std::time(nullptr)) + extension;
            break;
        }
    }
    
    return candidateName;
}

std::string AssetBrowserPanel::GenerateUniqueAssetName(const std::string& baseName, const std::string& directory) {
    std::string targetDir = directory.empty() ? m_currentDirectory : directory;
    
    fs::path baseNamePath(baseName);
    std::string stem = baseNamePath.stem().string();
    std::string extension = baseNamePath.extension().string();
    
    std::string candidateName = baseName;
    std::string candidatePath = targetDir + "/" + candidateName;
    int counter = 1;
    
    while (fs::exists(candidatePath)) {
        candidateName = stem + " " + std::to_string(counter) + extension;
        candidatePath = targetDir + "/" + candidateName;
        counter++;
        
        if (counter > 9999) {
            candidateName = stem + "_" + std::to_string(std::time(nullptr)) + extension;
            break;
        }
    }
    
    return candidateName;
}

bool AssetBrowserPanel::ValidateAssetName(const std::string& name, bool isDirectory) {
    if (name.empty()) {
        return false;
    }
    
    // Check for invalid characters
    const std::string invalidChars = "<>:\"/\\|?*";
    for (char c : invalidChars) {
        if (name.find(c) != std::string::npos) {
            return false;
        }
    }
    
    // Check for reserved names on Windows
    const std::vector<std::string> reservedNames = {
        "CON", "PRN", "AUX", "NUL", 
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    std::string upperName = name;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
    
    for (const auto& reserved : reservedNames) {
        if (upperName == reserved || upperName.find(reserved + ".") == 0) {
            return false;
        }
    }
    
    // Additional validation for files
    if (!isDirectory) {
        if (name.find('.') == std::string::npos) {
            return false; // Files should have extensions
        }
    }
    
    return true;
}

void AssetBrowserPanel::NotifyAssetSystemOfChanges(const std::string& assetPath, const std::string& operation) {
    // Refresh asset registry
    if (m_assetManager) {
        m_assetManager->GetRegistry().RefreshAsset(assetPath);
    }
    
    // Broadcast event for UI updates
    if (m_eventBus) {
        // Create a generic asset system event
        struct AssetSystemChangedEvent {
            std::string assetPath;
            std::string operation;
            AssetSystemChangedEvent(const std::string& path, const std::string& op) 
                : assetPath(path), operation(op) {}
        };
        
        m_eventBus->Publish(AssetSystemChangedEvent(assetPath, operation));
    }
}

// Multi-asset operations for professional workflow
bool AssetBrowserPanel::DeleteAssets(const std::vector<std::string>& paths) {
    bool allSuccessful = true;
    
    for (const auto& path : paths) {
        if (!DeleteAsset(path)) {
            allSuccessful = false;
        }
    }
    
    return allSuccessful;
}

// Clipboard Operations
void AssetBrowserPanel::CopyAssets(const std::vector<std::string>& assetPaths) {
    m_clipboardAssets = assetPaths;
    m_clipboardOperation = ClipboardOperation::Copy;
}

void AssetBrowserPanel::CutAssets(const std::vector<std::string>& assetPaths) {
    m_clipboardAssets = assetPaths;
    m_clipboardOperation = ClipboardOperation::Cut;
}

void AssetBrowserPanel::PasteAssets(const std::string& destinationDirectory) {
    if (m_clipboardOperation == ClipboardOperation::None || m_clipboardAssets.empty()) {
        return;
    }
    
    for (const auto& assetPath : m_clipboardAssets) {
        if (!fs::exists(assetPath)) {
            continue;
        }
        
        std::string fileName = fs::path(assetPath).filename().string();
        std::string uniqueName = GenerateUniqueAssetName(fileName, destinationDirectory);
        std::string destinationPath = destinationDirectory + "/" + uniqueName;
        
        try {
            if (m_clipboardOperation == ClipboardOperation::Copy) {
                // Copy operation
                if (fs::is_directory(assetPath)) {
                    fs::copy(assetPath, destinationPath, fs::copy_options::recursive);
                } else {
                    fs::copy_file(assetPath, destinationPath);
                }
                
                // Register with asset manager
                if (m_assetManager) {
                    m_assetManager->LoadAsset(destinationPath);
                }
                
                NotifyAssetSystemOfChanges(destinationPath, "copied");
                
            } else if (m_clipboardOperation == ClipboardOperation::Cut) {
                // Move operation
                MoveAsset(assetPath, destinationDirectory);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error pasting asset " << assetPath << ": " << e.what() << std::endl;
        }
    }
    
    // Clear clipboard after cut operation
    if (m_clipboardOperation == ClipboardOperation::Cut) {
        ClearClipboard();
    }
    
    RefreshCurrentDirectory();
}

bool AssetBrowserPanel::CanPaste() const {
    return m_clipboardOperation != ClipboardOperation::None && !m_clipboardAssets.empty();
}

void AssetBrowserPanel::ClearClipboard() {
    m_clipboardOperation = ClipboardOperation::None;
    m_clipboardAssets.clear();
}

} // namespace BGE
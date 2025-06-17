#include "ProjectSettingsPanel.h"
#include "../../ServiceLocator.h"
#include "../../../Renderer/Renderer.h"
#include "../../../ThirdParty/json/json.hpp"
#include "../../../ThirdParty/stb/stb_image.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace BGE {

ProjectSettingsPanel::ProjectSettingsPanel(const std::string& name) {
    m_windowName = name;
    m_projectRoot = fs::current_path().string();
    m_projectSettingsFile = m_projectRoot + "/ProjectSettings.json";
}

ProjectSettingsPanel::~ProjectSettingsPanel() {
    UnregisterEventListeners();
    
    // Clean up texture resources
    auto rendererPtr = ServiceLocator::Instance().GetService<Renderer>();
    if (rendererPtr) {
        for (auto& [type, settings] : m_assetTypeThumbnails) {
            if (settings.thumbnailTextureId != 0) {
                rendererPtr->DeleteTexture(settings.thumbnailTextureId);
            }
        }
    }
}

void ProjectSettingsPanel::Initialize() {
    // Get services
    m_eventBus = ServiceLocator::Instance().GetService<EventBus>().get();
    m_iconManager = &IconManager::Instance();
    
    RegisterEventListeners();
    LoadProjectSettings();
}

void ProjectSettingsPanel::RegisterEventListeners() {
    // No specific events to listen to for now
}

void ProjectSettingsPanel::UnregisterEventListeners() {
    // Event bus handles cleanup automatically
}

void ProjectSettingsPanel::Render() {
    if (!m_isVisible) {
        return;
    }
    
    // Set window size on first use
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    
    // Create standalone window with close button in title bar
    if (ImGui::Begin(m_windowName.c_str(), &m_isVisible, ImGuiWindowFlags_NoCollapse)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
        
        // No redundant header since window title already shows "Project Settings"
    
    // Two-column layout: sections list on left, content on right
    ImGui::BeginChild("SettingsLayout", ImVec2(0, -40), false, ImGuiWindowFlags_NoScrollbar);
    
    // Left panel: Sections list
    ImGui::BeginChild("SectionsList", ImVec2(m_sectionListWidth, 0), true);
    RenderSectionsList();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Splitter
    ImGui::Button("##Splitter", ImVec2(4, -1));
    if (ImGui::IsItemActive()) {
        m_sectionListWidth += ImGui::GetIO().MouseDelta.x;
        m_sectionListWidth = (std::max)(150.0f, (std::min)(300.0f, m_sectionListWidth));
    }
    
    ImGui::SameLine();
    
    // Right panel: Settings content
    ImGui::BeginChild("SettingsContent", ImVec2(0, 0), true);
    RenderSettingsContent();
    ImGui::EndChild();
    
    ImGui::EndChild();
    
    // Bottom buttons
    ImGui::Separator();
    
    // Save message feedback
    if (m_showSaveMessage) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Settings saved successfully!");
        m_saveMessageTimer -= ImGui::GetIO().DeltaTime;
        if (m_saveMessageTimer <= 0.0f) {
            m_showSaveMessage = false;
        }
    }
    
    if (ImGui::Button("Save & Apply")) {
        SaveProjectSettings();
        m_showSaveMessage = true;
        m_saveMessageTimer = 2.0f; // Show for 2 seconds
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Reset to Defaults")) {
        ResetToDefaults();
        m_showSaveMessage = true;
        m_saveMessageTimer = 2.0f;
    }
    
    
        ImGui::PopStyleVar(2);
    }
    ImGui::End();
}

void ProjectSettingsPanel::RenderSectionsList() {
    ImGui::Text("Settings");
    ImGui::Separator();
    
    auto sections = GetAllSections();
    for (SettingsSection section : sections) {
        const char* sectionName = GetSectionName(section);
        const char* sectionIcon = GetSectionIcon(section);
        
        bool isSelected = (m_currentSection == section);
        
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
        }
        
        std::string buttonLabel = std::string(sectionIcon) + " " + sectionName;
        if (ImGui::Button(buttonLabel.c_str(), ImVec2(-1, 0))) {
            m_currentSection = section;
        }
        
        if (isSelected) {
            ImGui::PopStyleColor();
        }
    }
}

void ProjectSettingsPanel::RenderSettingsContent() {
    switch (m_currentSection) {
        case SettingsSection::General:
            RenderGeneralSettings();
            break;
        case SettingsSection::AssetThumbnails:
            RenderAssetThumbnailsSettings();
            break;
        case SettingsSection::Rendering:
            RenderRenderingSettings();
            break;
        case SettingsSection::Input:
            RenderInputSettings();
            break;
        case SettingsSection::Audio:
            RenderAudioSettings();
            break;
        case SettingsSection::Performance:
            RenderPerformanceSettings();
            break;
    }
}

void ProjectSettingsPanel::RenderGeneralSettings() {
    ImGui::Text("General Settings");
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("Project Information:");
    ImGui::Indent();
    ImGui::Text("• Root Directory: %s", m_projectRoot.c_str());
    ImGui::Text("• Settings File: %s", fs::path(m_projectSettingsFile).filename().string().c_str());
    ImGui::Unindent();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextWrapped("Additional general settings will be added here in future updates, such as:");
    ImGui::BulletText("Project name and description");
    ImGui::BulletText("Version information");
    ImGui::BulletText("Build configurations");
    ImGui::BulletText("Default scene settings");
}

void ProjectSettingsPanel::RenderAssetThumbnailsSettings() {
    ImGui::Text("Asset Type Thumbnails");
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextWrapped("Customize the default thumbnail images for different asset types in the Asset Browser:");
    ImGui::Spacing();
    
    if (ImGui::BeginTable("AssetTypeThumbnails", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Asset Type", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Current Thumbnail", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Custom Path", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 150);
        ImGui::TableHeadersRow();
        
        auto assetTypes = GetAllAssetTypes();
        for (AssetType type : assetTypes) {
            const char* typeName = GetAssetTypeName(type);
            RenderAssetTypeRow(type, typeName);
        }
        
        ImGui::EndTable();
    }
}

void ProjectSettingsPanel::RenderRenderingSettings() {
    ImGui::Text("Rendering Settings");
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextWrapped("Configure rendering and graphics settings:");
    ImGui::Spacing();
    
    ImGui::BulletText("Graphics API preferences");
    ImGui::BulletText("Default shader settings");
    ImGui::BulletText("Texture compression options");
    ImGui::BulletText("Lighting and shadow settings");
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "These settings will be implemented in future updates.");
}

void ProjectSettingsPanel::RenderInputSettings() {
    ImGui::Text("Input Settings");
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextWrapped("Configure input handling and controls:");
    ImGui::Spacing();
    
    ImGui::BulletText("Default input mappings");
    ImGui::BulletText("Controller support");
    ImGui::BulletText("Mouse sensitivity");
    ImGui::BulletText("Keyboard shortcuts");
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "These settings will be implemented in future updates.");
}

void ProjectSettingsPanel::RenderAudioSettings() {
    ImGui::Text("Audio Settings");
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextWrapped("Configure audio system settings:");
    ImGui::Spacing();
    
    ImGui::BulletText("Audio device selection");
    ImGui::BulletText("Sample rate and buffer settings");
    ImGui::BulletText("Audio compression formats");
    ImGui::BulletText("3D audio settings");
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "These settings will be implemented in future updates.");
}

void ProjectSettingsPanel::RenderPerformanceSettings() {
    ImGui::Text("Performance Settings");
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextWrapped("Configure performance and optimization settings:");
    ImGui::Spacing();
    
    ImGui::BulletText("Memory allocation limits");
    ImGui::BulletText("Threading configuration");
    ImGui::BulletText("Asset streaming settings");
    ImGui::BulletText("Garbage collection options");
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "These settings will be implemented in future updates.");
}

void ProjectSettingsPanel::RenderAssetTypeRow(AssetType type, const char* typeName) {
    ImGui::TableNextRow();
    
    // Asset Type Name
    ImGui::TableNextColumn();
    ImGui::Text("%s", typeName);
    
    // Current Thumbnail
    ImGui::TableNextColumn();
    uint32_t thumbnailId = GetAssetTypeThumbnailTexture(type);
    if (thumbnailId != 0) {
        ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(thumbnailId)), ImVec2(48, 48));
    } else {
        // Show default icon
        std::string iconText = m_iconManager ? m_iconManager->GetIconText(type) : "?";
        ImGui::Button(iconText.c_str(), ImVec2(48, 48));
    }
    
    // Custom Path
    ImGui::TableNextColumn();
    auto it = m_assetTypeThumbnails.find(type);
    if (it != m_assetTypeThumbnails.end() && it->second.useCustomThumbnail) {
        ImGui::Text("%s", fs::path(it->second.customThumbnailPath).filename().string().c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", it->second.customThumbnailPath.c_str());
        }
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Default");
    }
    
    // Actions
    ImGui::TableNextColumn();
    ImGui::PushID(static_cast<int>(type));
    
    if (ImGui::Button("Set")) {
        m_fileDialogAssetType = type;
        std::string selectedFile = OpenNativeFileDialog();
        if (!selectedFile.empty()) {
            SetAssetTypeThumbnail(type, selectedFile);
        }
    }
    ImGui::SameLine();
    
    if (HasCustomThumbnail(type) && ImGui::Button("Remove")) {
        RemoveAssetTypeThumbnail(type);
    }
    
    ImGui::PopID();
}

void ProjectSettingsPanel::SetAssetTypeThumbnail(AssetType type, const std::string& imagePath) {
    if (!fs::exists(imagePath)) {
        std::cerr << "Thumbnail image does not exist: " << imagePath << std::endl;
        return;
    }
    
    if (LoadThumbnailTexture(type, imagePath)) {
        auto& settings = m_assetTypeThumbnails[type];
        settings.customThumbnailPath = imagePath;
        settings.useCustomThumbnail = true;
        
        // Broadcast settings changed event
        if (m_eventBus) {
            m_eventBus->Publish(ProjectSettingsChangedEvent("AssetTypeThumbnails"));
        }
        
        // Don't auto-save, user must press "Save & Apply"
    }
}

void ProjectSettingsPanel::RemoveAssetTypeThumbnail(AssetType type) {
    auto it = m_assetTypeThumbnails.find(type);
    if (it != m_assetTypeThumbnails.end()) {
        // Clean up texture
        if (it->second.thumbnailTextureId != 0) {
            auto rendererPtr = ServiceLocator::Instance().GetService<Renderer>();
            if (rendererPtr) {
                rendererPtr->DeleteTexture(it->second.thumbnailTextureId);
            }
        }
        
        it->second.useCustomThumbnail = false;
        it->second.customThumbnailPath.clear();
        it->second.thumbnailTextureId = 0;
        
        // Broadcast settings changed event
        if (m_eventBus) {
            m_eventBus->Publish(ProjectSettingsChangedEvent("AssetTypeThumbnails"));
        }
        
        // Don't auto-save, user must press "Save & Apply"
    }
}

std::string ProjectSettingsPanel::GetAssetTypeThumbnailPath(AssetType type) const {
    auto it = m_assetTypeThumbnails.find(type);
    if (it != m_assetTypeThumbnails.end() && it->second.useCustomThumbnail) {
        return it->second.customThumbnailPath;
    }
    return "";
}

uint32_t ProjectSettingsPanel::GetAssetTypeThumbnailTexture(AssetType type) const {
    auto it = m_assetTypeThumbnails.find(type);
    if (it != m_assetTypeThumbnails.end() && it->second.useCustomThumbnail) {
        return it->second.thumbnailTextureId;
    }
    return 0;
}

bool ProjectSettingsPanel::HasCustomThumbnail(AssetType type) const {
    auto it = m_assetTypeThumbnails.find(type);
    return it != m_assetTypeThumbnails.end() && it->second.useCustomThumbnail;
}

bool ProjectSettingsPanel::LoadThumbnailTexture(AssetType type, const std::string& imagePath) {
    std::cout << "Loading thumbnail: " << imagePath << std::endl;
    
    int width, height, originalChannels;
    // Force load as RGBA (4 channels) for consistency
    unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &originalChannels, 4);
    
    if (!data) {
        std::cerr << "Failed to load thumbnail image: " << imagePath << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    std::cout << "Image loaded: " << width << "x" << height << " original channels: " << originalChannels << " (converted to RGBA)" << std::endl;
    
    // Get renderer service to create texture
    auto rendererPtr = ServiceLocator::Instance().GetService<Renderer>();
    if (!rendererPtr) {
        std::cerr << "Renderer service not available" << std::endl;
        stbi_image_free(data);
        return false;
    }
    
    // Create texture with RGBA format
    uint32_t textureId = rendererPtr->CreateTexture(width, height, 4, data);
    
    std::cout << "Created texture ID: " << textureId << std::endl;
    
    stbi_image_free(data);
    
    if (textureId == 0) {
        std::cerr << "Failed to create texture for thumbnail" << std::endl;
        return false;
    }
    
    // Clean up old texture if it exists
    auto& settings = m_assetTypeThumbnails[type];
    if (settings.thumbnailTextureId != 0) {
        rendererPtr->DeleteTexture(settings.thumbnailTextureId);
    }
    
    settings.thumbnailTextureId = textureId;
    std::cout << "Thumbnail texture set successfully for asset type " << static_cast<int>(type) << std::endl;
    return true;
}

void ProjectSettingsPanel::LoadProjectSettings() {
    if (!fs::exists(m_projectSettingsFile)) {
        return; // No settings file yet, use defaults
    }
    
    try {
        std::ifstream file(m_projectSettingsFile);
        json projectData;
        file >> projectData;
        
        // Load asset type thumbnails
        if (projectData.contains("assetTypeThumbnails")) {
            for (const auto& [typeStr, thumbnailData] : projectData["assetTypeThumbnails"].items()) {
                AssetType type = static_cast<AssetType>(std::stoi(typeStr));
                
                if (thumbnailData.contains("customThumbnailPath") && 
                    thumbnailData.contains("useCustomThumbnail") &&
                    thumbnailData["useCustomThumbnail"].get<bool>()) {
                    
                    std::string thumbnailPath = thumbnailData["customThumbnailPath"];
                    if (fs::exists(thumbnailPath)) {
                        SetAssetTypeThumbnail(type, thumbnailPath);
                    }
                }
            }
        }
        
        // Load individual asset thumbnails
        if (projectData.contains("individualAssetThumbnails")) {
            for (const auto& [assetHandleStr, thumbnailData] : projectData["individualAssetThumbnails"].items()) {
                std::string imagePath;
                std::string assetPath;
                
                if (thumbnailData.is_string()) {
                    // Legacy format - just the image path
                    imagePath = thumbnailData.get<std::string>();
                    assetPath = ""; // No path info in legacy format
                } else if (thumbnailData.is_object()) {
                    // New format with path info
                    imagePath = thumbnailData.value("imagePath", "");
                    assetPath = thumbnailData.value("assetPath", "");
                }
                
                if (!imagePath.empty() && fs::exists(imagePath)) {
                    // Load the image and create texture
                    int width, height, channels;
                    unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);
                    
                    if (data) {
                        auto rendererPtr = ServiceLocator::Instance().GetService<Renderer>();
                        if (rendererPtr) {
                            uint32_t textureId = rendererPtr->CreateTexture(width, height, 4, data);
                            if (textureId != 0) {
                                AssetHandle handle = AssetHandle::FromString(assetHandleStr);
                                IndividualAssetThumbnail thumbnail = {textureId, imagePath, assetPath};
                                m_individualAssetThumbnails[handle] = thumbnail;
                                if (!assetPath.empty()) {
                                    m_pathBasedThumbnails[assetPath] = thumbnail;
                                }
                            }
                        }
                        stbi_image_free(data);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading project settings: " << e.what() << std::endl;
    }
}

void ProjectSettingsPanel::SaveProjectSettings() {
    try {
        json projectData;
        
        // Save asset type thumbnails
        json thumbnailsData;
        for (const auto& [type, settings] : m_assetTypeThumbnails) {
            if (settings.useCustomThumbnail) {
                json thumbnailInfo;
                thumbnailInfo["customThumbnailPath"] = settings.customThumbnailPath;
                thumbnailInfo["useCustomThumbnail"] = settings.useCustomThumbnail;
                
                thumbnailsData[std::to_string(static_cast<int>(type))] = thumbnailInfo;
            }
        }
        projectData["assetTypeThumbnails"] = thumbnailsData;
        
        // Save individual asset thumbnails
        json individualThumbnailsData;
        for (const auto& [assetHandle, thumbnail] : m_individualAssetThumbnails) {
            if (thumbnail.textureId != 0 && !thumbnail.sourcePath.empty()) {
                json thumbnailInfo;
                thumbnailInfo["imagePath"] = thumbnail.sourcePath;
                thumbnailInfo["assetPath"] = thumbnail.assetPath;
                individualThumbnailsData[assetHandle.ToString()] = thumbnailInfo;
            }
        }
        if (!individualThumbnailsData.empty()) {
            projectData["individualAssetThumbnails"] = individualThumbnailsData;
        }
        
        // Write to file
        std::ofstream file(m_projectSettingsFile);
        file << projectData.dump(2);
        file.close();
        
        std::cout << "Project settings saved to: " << m_projectSettingsFile << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving project settings: " << e.what() << std::endl;
    }
}

void ProjectSettingsPanel::ResetToDefaults() {
    // Clear all custom thumbnails
    auto rendererPtr = ServiceLocator::Instance().GetService<Renderer>();
    for (auto& [type, settings] : m_assetTypeThumbnails) {
        if (settings.thumbnailTextureId != 0 && rendererPtr) {
            rendererPtr->DeleteTexture(settings.thumbnailTextureId);
        }
        settings.useCustomThumbnail = false;
        settings.customThumbnailPath.clear();
        settings.thumbnailTextureId = 0;
    }
    
    // Clear individual asset thumbnails
    for (auto& [assetHandle, thumbnail] : m_individualAssetThumbnails) {
        if (thumbnail.textureId != 0 && rendererPtr) {
            rendererPtr->DeleteTexture(thumbnail.textureId);
        }
    }
    m_individualAssetThumbnails.clear();
    m_pathBasedThumbnails.clear();
    
    // Broadcast settings changed event
    if (m_eventBus) {
        m_eventBus->Publish(ProjectSettingsChangedEvent("AssetTypeThumbnails"));
    }
}

std::string ProjectSettingsPanel::GetProjectSettingsPath() const {
    return m_projectSettingsFile;
}

void ProjectSettingsPanel::SetIndividualAssetThumbnail(const AssetHandle& assetHandle, const std::string& assetPath, uint32_t textureId, const std::string& sourcePath) {
    IndividualAssetThumbnail thumbnail = {textureId, sourcePath, assetPath};
    m_individualAssetThumbnails[assetHandle] = thumbnail;
    m_pathBasedThumbnails[assetPath] = thumbnail; // Also store by path for backup lookup
}

void ProjectSettingsPanel::RemoveIndividualAssetThumbnail(const AssetHandle& assetHandle) {
    auto it = m_individualAssetThumbnails.find(assetHandle);
    if (it != m_individualAssetThumbnails.end()) {
        // Clean up texture
        if (it->second.textureId != 0) {
            auto rendererPtr = ServiceLocator::Instance().GetService<Renderer>();
            if (rendererPtr) {
                rendererPtr->DeleteTexture(it->second.textureId);
            }
        }
        
        // Remove from path-based mapping too
        const std::string& assetPath = it->second.assetPath;
        m_pathBasedThumbnails.erase(assetPath);
        
        m_individualAssetThumbnails.erase(it);
    }
}

void ProjectSettingsPanel::RemoveIndividualAssetThumbnailByPath(const std::string& assetPath) {
    auto pathIt = m_pathBasedThumbnails.find(assetPath);
    if (pathIt != m_pathBasedThumbnails.end()) {
        // Clean up texture
        if (pathIt->second.textureId != 0) {
            auto rendererPtr = ServiceLocator::Instance().GetService<Renderer>();
            if (rendererPtr) {
                rendererPtr->DeleteTexture(pathIt->second.textureId);
            }
        }
        
        // Find and remove from handle-based mapping
        for (auto it = m_individualAssetThumbnails.begin(); it != m_individualAssetThumbnails.end(); ++it) {
            if (it->second.assetPath == assetPath) {
                m_individualAssetThumbnails.erase(it);
                break;
            }
        }
        
        m_pathBasedThumbnails.erase(pathIt);
    }
}

uint32_t ProjectSettingsPanel::GetIndividualAssetThumbnail(const AssetHandle& assetHandle) const {
    auto it = m_individualAssetThumbnails.find(assetHandle);
    if (it != m_individualAssetThumbnails.end()) {
        return it->second.textureId;
    }
    return 0;
}

uint32_t ProjectSettingsPanel::GetIndividualAssetThumbnailByPath(const std::string& assetPath) const {
    auto it = m_pathBasedThumbnails.find(assetPath);
    if (it != m_pathBasedThumbnails.end()) {
        return it->second.textureId;
    }
    return 0;
}

void ProjectSettingsPanel::UpdateAssetThumbnailMapping(const AssetHandle& oldHandle, const AssetHandle& newHandle, const std::string& newPath) {
    auto it = m_individualAssetThumbnails.find(oldHandle);
    if (it != m_individualAssetThumbnails.end()) {
        // Move thumbnail to new handle
        IndividualAssetThumbnail thumbnail = it->second;
        thumbnail.assetPath = newPath; // Update path
        
        m_individualAssetThumbnails[newHandle] = thumbnail;
        m_pathBasedThumbnails[newPath] = thumbnail;
        
        // Remove old mappings
        m_pathBasedThumbnails.erase(it->second.assetPath);
        m_individualAssetThumbnails.erase(it);
    }
}

void ProjectSettingsPanel::RestoreThumbnailFromPath(const AssetHandle& assetHandle, const std::string& assetPath) {
    auto pathIt = m_pathBasedThumbnails.find(assetPath);
    if (pathIt != m_pathBasedThumbnails.end() && assetHandle.IsValid()) {
        // Asset was moved or recreated - restore the mapping
        IndividualAssetThumbnail thumbnail = pathIt->second;
        thumbnail.assetPath = assetPath; // Ensure path is current
        m_individualAssetThumbnails[assetHandle] = thumbnail;
    }
}

const char* ProjectSettingsPanel::GetAssetTypeName(AssetType type) const {
    switch (type) {
        case AssetType::Texture: return "Texture";
        case AssetType::Material: return "Material";
        case AssetType::Scene: return "Scene";
        case AssetType::Audio: return "Audio";
        case AssetType::Script: return "Script";
        case AssetType::Prefab: return "Prefab";
        case AssetType::Model: return "Model";
        case AssetType::Animation: return "Animation";
        case AssetType::Folder: return "Folder";
        default: return "Unknown";
    }
}

std::vector<AssetType> ProjectSettingsPanel::GetAllAssetTypes() const {
    return {
        AssetType::Texture,
        AssetType::Material,
        AssetType::Scene,
        AssetType::Audio,
        AssetType::Script,
        AssetType::Prefab,
        AssetType::Model,
        AssetType::Animation,
        AssetType::Folder
    };
}

const char* ProjectSettingsPanel::GetSectionName(SettingsSection section) const {
    switch (section) {
        case SettingsSection::General: return "General";
        case SettingsSection::AssetThumbnails: return "Asset Thumbnails";
        case SettingsSection::Rendering: return "Rendering";
        case SettingsSection::Input: return "Input";
        case SettingsSection::Audio: return "Audio";
        case SettingsSection::Performance: return "Performance";
        default: return "Unknown";
    }
}

const char* ProjectSettingsPanel::GetSectionIcon(SettingsSection section) const {
    switch (section) {
        case SettingsSection::General: return "[G]";
        case SettingsSection::AssetThumbnails: return "[T]";
        case SettingsSection::Rendering: return "[R]";
        case SettingsSection::Input: return "[I]";
        case SettingsSection::Audio: return "[A]";
        case SettingsSection::Performance: return "[P]";
        default: return "[?]";
    }
}

std::vector<SettingsSection> ProjectSettingsPanel::GetAllSections() const {
    return {
        SettingsSection::General,
        SettingsSection::AssetThumbnails,
        SettingsSection::Rendering,
        SettingsSection::Input,
        SettingsSection::Audio,
        SettingsSection::Performance
    };
}

std::string ProjectSettingsPanel::OpenNativeFileDialog() {
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0PNG Files\0*.png\0JPEG Files\0*.jpg;*.jpeg\0BMP Files\0*.bmp\0TGA Files\0*.tga\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn)) {
        return std::string(szFile);
    }
    
    return "";
#else
    // For non-Windows platforms, could use zenity, kdialog, or a cross-platform library
    std::cerr << "Native file dialog not implemented for this platform" << std::endl;
    return "";
#endif
}

} // namespace BGE
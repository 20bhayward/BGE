#include "ConsolePanel.h"
#include <imgui.h>
#include <ctime>

namespace BGE {

ConsolePanel::ConsolePanel(const std::string& name)
    : Panel(name) {
}

void ConsolePanel::Initialize() {
    SetMinSize(300, 150);
    AddLog("Console initialized");
}

void ConsolePanel::OnRender() {
    // Console output
    const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    for (const auto& log : m_logs) {
        ImGui::TextUnformatted(log.c_str());
    }
    
    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    ImGui::Separator();
    
    // Command input
    bool reclaimFocus = false;
    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    
    if (ImGui::InputText("Input", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), inputFlags)) {
        if (m_inputBuffer[0]) {
            AddLog(std::string("> ") + m_inputBuffer);
            // Process command here
            AddLog("Command not implemented yet");
        }
        strcpy(m_inputBuffer, "");
        reclaimFocus = true;
    }
    
    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaimFocus) {
        ImGui::SetKeyboardFocusHere(-1);
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        Clear();
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
}

void ConsolePanel::AddLog(const std::string& message) {
    // Add timestamp
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    
    m_logs.push_back(std::string("[") + timeStr + "] " + message);
    
    // Limit log size
    if (m_logs.size() > 1000) {
        m_logs.erase(m_logs.begin());
    }
}

void ConsolePanel::Clear() {
    m_logs.clear();
}

} // namespace BGE
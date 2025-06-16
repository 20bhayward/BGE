#pragma once

#include "../Framework/Panel.h"
#include <vector>
#include <string>

namespace BGE {

class ConsolePanel : public Panel {
public:
    ConsolePanel(const std::string& name);
    
    void Initialize() override;
    void OnRender() override;
    
    void AddLog(const std::string& message);
    void Clear();
    
private:
    std::vector<std::string> m_logs;
    bool m_autoScroll = true;
    char m_inputBuffer[256] = {};
};

} // namespace BGE
#pragma once

#include <vector>
#include <functional>

namespace BGE {

enum class CommandType {
    Draw,
    DrawIndexed,
    SetPipeline,
    SetVertexBuffer,
    SetIndexBuffer,
    SetUniformBuffer,
    SetTexture,
    BeginRenderPass,
    EndRenderPass
};

struct RenderCommand {
    CommandType type;
    std::function<void()> execute;
};

class CommandBuffer {
public:
    CommandBuffer();
    ~CommandBuffer();
    
    void Begin();
    void End();
    void Reset();
    
    void AddCommand(CommandType type, std::function<void()> command);
    void Execute();
    
    bool IsRecording() const { return m_recording; }
    size_t GetCommandCount() const { return m_commands.size(); }

private:
    std::vector<RenderCommand> m_commands;
    bool m_recording;
};

} // namespace BGE
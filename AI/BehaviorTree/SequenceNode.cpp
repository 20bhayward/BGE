// AI/BehaviorTree/SequenceNode.cpp
#include "AI/BehaviorTree/SequenceNode.h"

namespace BGE {

BTStatus SequenceNode::Tick() {
    for (BTNode* child : m_children) {
        if (!child) continue; // Should not happen if AddChild prevents nulls

        BTStatus status = child->Tick();
        if (status == BTStatus::Failure) {
            return BTStatus::Failure;
        }
        if (status == BTStatus::Running) {
            return BTStatus::Running;
        }
        // If status is Success, continue to the next child
    }
    return BTStatus::Success; // All children succeeded
}

} // namespace BGE

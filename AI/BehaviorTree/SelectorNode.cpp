// AI/BehaviorTree/SelectorNode.cpp
#include "AI/BehaviorTree/SelectorNode.h"

namespace BGE {

BTStatus SelectorNode::Tick() {
    for (BTNode* child : m_children) {
        if (!child) continue;

        BTStatus status = child->Tick();
        if (status == BTStatus::Success) {
            return BTStatus::Success;
        }
        if (status == BTStatus::Running) {
            return BTStatus::Running;
        }
        // If status is Failure, try the next child
    }
    return BTStatus::Failure; // All children failed
}

} // namespace BGE

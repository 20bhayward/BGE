// AI/BehaviorTree/BehaviorTree.cpp
#include "AI/BehaviorTree/BehaviorTree.h" // Changed from BehaviorTree/BehaviorTree.h

namespace BGE {

BehaviorTree::BehaviorTree(BTNode* rootNode) : m_rootNode(rootNode) {}

BehaviorTree::~BehaviorTree() {
    // It's generally a good idea for the BehaviorTree to own its root node
    // and be responsible for its deletion, assuming the root node
    // then handles deletion of its children, and so on.
    // However, direct deletion here might be too simplistic if nodes are shared.
    // For now, we'll delete the root. Consider a more robust memory management strategy
    // (e.g., smart pointers or explicit cleanup methods) for complex trees.
    delete m_rootNode;
}

BTStatus BehaviorTree::Tick() {
    if (m_rootNode) {
        return m_rootNode->Tick();
    }
    return BTStatus::Failure; // Or some other appropriate status if there's no root
}

} // namespace BGE

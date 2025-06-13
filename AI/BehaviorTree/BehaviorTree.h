// AI/BehaviorTree/BehaviorTree.h
#ifndef BEHAVIORTREE_H
#define BEHAVIORTREE_H

#include "AI/BehaviorTree/BTNode.h" // Changed from BehaviorTree/BTNode.h

namespace BGE {

class BehaviorTree {
public:
    BehaviorTree(BTNode* rootNode);
    ~BehaviorTree(); // Added destructor to manage rootNode memory

    BTStatus Tick();

private:
    BTNode* m_rootNode;
};

} // namespace BGE

#endif // BEHAVIORTREE_H

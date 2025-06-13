// AI/BehaviorTree/CompositeNode.h
#ifndef COMPOSITENODE_H
#define COMPOSITENODE_H

#include "AI/BehaviorTree/BTNode.h"
#include <vector>
#include <algorithm>

namespace BGE {

class CompositeNode : public BTNode {
public:
    virtual ~CompositeNode(); // Declare destructor

    void AddChild(BTNode* child); // Declare AddChild

protected:
    CompositeNode(); // Declare protected constructor

    std::vector<BTNode*> m_children;
};

} // namespace BGE

#endif // COMPOSITENODE_H

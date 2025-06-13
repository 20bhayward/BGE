// AI/BehaviorTree/SelectorNode.h
#ifndef SELECTORNODE_H
#define SELECTORNODE_H

#include "AI/BehaviorTree/CompositeNode.h"

namespace BGE {

class SelectorNode : public CompositeNode {
public:
    SelectorNode() = default;
    virtual ~SelectorNode() = default;

    BTStatus Tick() override;
};

} // namespace BGE

#endif // SELECTORNODE_H

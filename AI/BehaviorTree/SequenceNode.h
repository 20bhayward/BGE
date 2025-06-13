// AI/BehaviorTree/SequenceNode.h
#ifndef SEQUENCENODE_H
#define SEQUENCENODE_H

#include "AI/BehaviorTree/CompositeNode.h"

namespace BGE {

class SequenceNode : public CompositeNode {
public:
    SequenceNode() = default;
    virtual ~SequenceNode() = default;

    BTStatus Tick() override;
};

} // namespace BGE

#endif // SEQUENCENODE_H

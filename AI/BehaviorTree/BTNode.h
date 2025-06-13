// AI/BehaviorTree/BTNode.h
#ifndef BTNODE_H
#define BTNODE_H

namespace BGE {

enum class BTStatus {
    Success,
    Failure,
    Running,
};

class BTNode {
public:
    virtual ~BTNode() = default;
    virtual BTStatus Tick() = 0;
};

} // namespace BGE

#endif // BTNODE_H

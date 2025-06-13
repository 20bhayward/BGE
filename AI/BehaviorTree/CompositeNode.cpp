// AI/BehaviorTree/CompositeNode.cpp
#include "AI/BehaviorTree/CompositeNode.h"

namespace BGE {

CompositeNode::CompositeNode() = default; // Definition for protected constructor

CompositeNode::~CompositeNode() {
    for (BTNode* child : m_children) {
        delete child;
    }
    // m_children.clear(); // Not strictly necessary as vector will be destroyed
}

void CompositeNode::AddChild(BTNode* child) {
    if (child) {
        m_children.push_back(child);
    }
}

} // namespace BGE

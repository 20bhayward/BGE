#include "ServiceLocator.h"

namespace BGE {

ServiceLocator& ServiceLocator::Instance() {
    static ServiceLocator instance;
    return instance;
}

void ServiceLocator::Clear() {
    m_services.clear();
}

} // namespace BGE